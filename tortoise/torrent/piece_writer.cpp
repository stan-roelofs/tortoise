#include "piece_writer.hpp"

#include <cassert>
#include <format>

#include <tortoise/exceptions.hpp>

#include "../util/log.hpp"

namespace
{
	const std::string log_tag = "PieceWriter";
}

namespace tortoise
{
	PieceWriter::PieceWriter(const Metainfo &metainfo, std::filesystem::path output_path, Callbacks callbacks)
		: callbacks_(std::move(callbacks)),
		  pieces_done_(0),
		  output_path_(std::move(output_path))
	{
		if (metainfo.files.empty())
			throw InvalidArgumentException("Invalid metainfo");

		piece_to_file_.resize(metainfo.pieces.size());

		// TODO: if files already exist, support reading them (we probably shouldnt truncate them in the next step!!)
		const std::ios::openmode mode = std::ios::out | std::ios::trunc | std::ios::binary;

		// Prepare files/directories
		std::vector<std::shared_ptr<FileInfo>> filehandles;
		filestreams_.reserve(metainfo.files.size());
		for (const auto &file : metainfo.files)
		{
			auto parent = file.path;
			parent.remove_filename();
			if (!parent.empty())
			{
				std::error_code ec;
				std::filesystem::create_directories(parent, ec);
				if (ec)
				{
					Error(std::format("Creating directories failed {}", parent.string()));
					return;
				}
			}

			std::shared_ptr<FileInfo> fi = std::make_shared<FileInfo>();
			filestreams_.emplace_back(file.path, mode);
			fi->stream = &filestreams_.back();
			fi->stream->seekp(file.length - 1, std::ios::beg);
			fi->stream->write("", 1);
			fi->stream->flush();
			fi->path = file.path;
			filehandles.push_back(fi);

			if (!fi->stream->good())
			{
				Error(std::format("Creating file {} failed", file.path.string()));
				return;
			}
		}

		std::size_t current_file = 0;
		std::size_t current_file_size_left = metainfo.files[current_file].length;
		for (std::size_t piece_index = 0; piece_index < metainfo.pieces.size(); ++piece_index)
		{
			std::size_t piece_size_left = metainfo.piece_length;
			do
			{
				FilePiece filepiece;
				filepiece.file = filehandles.at(current_file);
				filepiece.offset = metainfo.files.at(current_file).length - current_file_size_left;
				filepiece.length = std::min(current_file_size_left, piece_size_left);

				piece_size_left -= filepiece.length;
				current_file_size_left -= filepiece.length;

				piece_to_file_[piece_index].push_back(filepiece);

				if (current_file_size_left == 0)
				{
					++current_file;
					if (current_file == metainfo.files.size())
						break;
					current_file_size_left = metainfo.files.at(current_file).length;
				}

			} while (piece_size_left > 0 && current_file != metainfo.files.size());
		}

		thread_ = std::jthread([this](std::stop_token stop_token)
							   { Run(stop_token); });
	}

	PieceWriter::~PieceWriter()
	{
		Stop();
		cv_.notify_all();

		if (thread_.joinable())
			thread_.join();
		Flush();
	}

	void PieceWriter::WritePiece(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data)
	{
		std::scoped_lock lock(mutex_);

		if (!data || piece_index >= piece_to_file_.size())
			throw InvalidArgumentException("Invalid piece index or data");

		LOG_DEBUG(log_tag, std::format("Scheduled piece {} for writing", piece_index));
		pieces_queue_.emplace(piece_index, std::move(data));
		cv_.notify_one();
	}

	void PieceWriter::Error(const std::string &error)
	{
		LOG_ERROR(log_tag, error);
		if (callbacks_.on_error)
			callbacks_.on_error();
	}

	void PieceWriter::Run(std::stop_token stop_token)
	{
		bool error = false;

		const auto ReportError = [this, &error](const std::string &message)
		{
			Error(message);
			error = true;
		};

		while (!stop_token.stop_requested() && !error)
		{
			std::unique_lock lock(mutex_);
			cv_.wait(lock);

			while (!pieces_queue_.empty() && !error)
			{
				auto piece = pieces_queue_.front();
				pieces_queue_.pop();
				lock.unlock();

				std::size_t data_offset = 0;
				for (auto &filepiece : piece_to_file_.at(piece.index))
				{
					LOG_INFO(log_tag, std::format("Writing piece {} to file {} at offset {}", piece.index, filepiece.file->path.string(), filepiece.offset));
					filepiece.file->stream->seekp(filepiece.offset);
					if (data_offset + filepiece.length > piece.data->size())
					{
						LOG_ERROR(log_tag, std::format("Invalid piece data"));
						throw Exception("Internal error");
					}
					if (!filepiece.file->stream->write((char *)piece.data->data() + data_offset, filepiece.length))
						ReportError(std::format("Writing data to file {} failed", filepiece.file->path.string()));
					data_offset += filepiece.length;
				}

				++pieces_done_;

				lock.lock();
			}

			if (pieces_done_ == piece_to_file_.size())
			{
				Flush();
				Stop();
				if (callbacks_.on_done)
					callbacks_.on_done();
			}
		}
	}
	void PieceWriter::Flush()
	{
		LOG_INFO(log_tag, std::format("Flushing {} files", filestreams_.size()));
		for (auto &file : filestreams_)
			file.flush();
	}
	void PieceWriter::Stop()
	{
		LOG_INFO(log_tag, "Stopping");
		thread_.request_stop();
	}
} // namespace tortoise