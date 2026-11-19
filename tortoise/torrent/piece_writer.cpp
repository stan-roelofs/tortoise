#include "piece_writer.hpp"

#include <cassert>
#include <format>

#include <mutex>
#include <tortoise/exception.hpp>

#include "../util/log.hpp"

namespace {
const std::string log_tag = "PieceWriter";
} // namespace

namespace tortoise {
PieceWriter::PieceWriter(std::shared_ptr<const Metainfo> metainfo, const std::filesystem::path &output_path)
    : metainfo_(std::move(metainfo)), output_path_(std::move(output_path)), state_(State::Preparing), pieces_done_(0) {
  if (!metainfo_ || metainfo_->files.empty())
    throw InvalidArgumentException("Invalid metainfo");

  thread_ = std::jthread([this](std::stop_token stop_token) { Run(stop_token); });
}

PieceWriter::~PieceWriter() {
  Stop();
  pieces_cv_.notify_all();

  if (thread_.joinable())
    thread_.join();
  Flush();
}

void PieceWriter::WritePiece(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data) {
  std::scoped_lock lock(pieces_mutex_);

  if (!data || piece_index >= piece_to_file_.size())
    throw InvalidArgumentException("Invalid piece index or data");

  LOG_DEBUG(log_tag, std::format("Scheduled piece {} for writing", piece_index));
  pieces_queue_.emplace(piece_index, std::move(data));
  pieces_cv_.notify_one();
}

PieceWriter::State PieceWriter::GetState() const {
  std::scoped_lock lock(state_mutex_);
  return state_;
}

void PieceWriter::SetState(State state) {
  std::scoped_lock lock(state_mutex_);
  if (state_ == state)
    return;
  LOG_INFO(log_tag, std::format("State changed from {} to {}", static_cast<int>(state_), static_cast<int>(state)));
  state_ = state;
}

void PieceWriter::Error(const std::string &error) {
  LOG_ERROR(log_tag, error);
  SetState(State::Error);
  Stop();
}

void PieceWriter::Run(std::stop_token stop_token) {
  while (!stop_token.stop_requested()) {
    switch (GetState()) {
    case State::Preparing:
      State_Prepare(stop_token);
      break;
    case State::Writing:
      State_Write(stop_token);
      break;
    case State::Done:
    case State::Error:
      Stop();
      break;
    }
  }
}
void PieceWriter::Flush() {
  LOG_INFO(log_tag, std::format("Flushing {} files", filestreams_.size()));
  for (auto &file : filestreams_)
    file.flush();
}
void PieceWriter::Stop() {
  LOG_INFO(log_tag, "Stopping");
  thread_.request_stop();
}

static bool CreateDirectories(const std::filesystem::path &path) {
  LOG_INFO(log_tag, std::format("Creating directories for {}", path.string()));
  const auto parent = path.parent_path();
  if (parent.empty())
    return true;
  std::error_code ec;
  std::filesystem::create_directories(path, ec);
  if (ec) {
    LOG_ERROR(log_tag, std::format("Creating directories failed {}: {}", parent.string(), ec.message()));
    return false;
  }
  return true;
}

void PieceWriter::State_Prepare(const std::stop_token &stop_token) {
  LOG_INFO(log_tag, "Preparing files for writing");
  piece_to_file_.resize(metainfo_->pieces.size());

  std::vector<std::shared_ptr<FileInfo>> filehandles;
  if (!PrepareFileStreams(stop_token, filehandles)) {
    LOG_ERROR(log_tag, "Preparing file streams failed or thread was stopped");
    SetState(State::Error);
    return;
  }

  if (!PrepareFilePieces(stop_token, filehandles)) {
    LOG_ERROR(log_tag, "Preparing file pieces failed or thread was stopped");
    SetState(State::Error);
    return;
  }

  if (!ReadExistingFiles(stop_token)) {
    LOG_ERROR(log_tag, "Reading existing files failed or thread was stopped");
    SetState(State::Error);
    return;
  }

  SetState(State::Writing);
}

void PieceWriter::State_Write(const std::stop_token &stop_token) {
  std::unique_lock lock(pieces_mutex_);
  pieces_cv_.wait(lock);

  while (!pieces_queue_.empty() && !stop_token.stop_requested()) {
    auto piece = pieces_queue_.front();
    pieces_queue_.pop();
    lock.unlock();

    std::size_t data_offset = 0;
    for (auto &filepiece : piece_to_file_.at(piece.index)) {
      LOG_INFO(log_tag,
               std::format("Writing piece {} to file {} at offset {}", piece.index, filepiece.file->path.string(), filepiece.offset));
      filepiece.file->stream->seekp(filepiece.offset);

      const std::size_t piece_size = piece.data->size();
      if (data_offset + filepiece.length > piece_size) {
        LOG_ERROR(log_tag, std::format("Invalid piece data"));
        throw Exception("Internal error");
      }

      // TODO: write in small chunks such that this doesn't block for too long...
      if (!filepiece.file->stream->write((char *)piece.data->data() + data_offset, filepiece.length)) {
        Error(std::format("Writing data to file {} failed", filepiece.file->path.string()));
        SetState(State::Error);
        return;
      }
      data_offset += filepiece.length;

      if (stop_token.stop_requested())
        break;
    }

    ++pieces_done_;

    lock.lock();
  }

  if (pieces_done_ == piece_to_file_.size()) {
    Flush();
    SetState(State::Done);
    Stop();
  }
}
bool PieceWriter::PrepareFileStreams(const std::stop_token &stop_token, std::vector<std::shared_ptr<FileInfo>> &filehandles) {
  filestreams_.reserve(metainfo_->files.size());

  const std::ios::openmode mode = std::ios::in | std::ios::out | std::ios::binary;

  // Prepare files/directories
  for (const auto &file : metainfo_->files) {
    if (!CreateDirectories(file.path)) {
      Error(std::format("Creating directories failed for {}", file.path.string()));
      return false;
    }

    std::shared_ptr<FileInfo> fi = std::make_shared<FileInfo>();
    filestreams_.emplace_back(file.path, mode);
    fi->stream = &filestreams_.back();
    fi->path = file.path;
    filehandles.push_back(fi);

    if (!fi->stream->good()) {
      Error(std::format("Creating file {} failed", file.path.string()));
      return false;
    }

    if (stop_token.stop_requested())
      return false;
  }
  return true;
}

bool PieceWriter::PrepareFilePieces(const std::stop_token &stop_token, const std::vector<std::shared_ptr<FileInfo>> &filehandles) {
  std::size_t current_file = 0;
  std::size_t current_file_size_left = metainfo_->files[current_file].length;
  for (std::size_t piece_index = 0; piece_index < metainfo_->pieces.size(); ++piece_index) {
    std::size_t piece_size_left = metainfo_->piece_length;
    do {
      FilePiece filepiece;
      filepiece.file = filehandles.at(current_file);
      filepiece.offset = metainfo_->files.at(current_file).length - current_file_size_left;
      filepiece.length = std::min(current_file_size_left, piece_size_left);

      piece_size_left -= filepiece.length;
      current_file_size_left -= filepiece.length;

      piece_to_file_[piece_index].push_back(filepiece);

      if (current_file_size_left == 0) {
        ++current_file;
        if (current_file == metainfo_->files.size())
          break;
        current_file_size_left = metainfo_->files.at(current_file).length;
      }

    } while (piece_size_left > 0 && current_file != metainfo_->files.size() && !stop_token.stop_requested());
  }

  return !stop_token.stop_requested();
}
bool PieceWriter::ReadExistingFiles(const std::stop_token &) { return true; }
} // namespace tortoise