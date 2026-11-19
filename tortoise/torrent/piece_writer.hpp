#ifndef TORTOISE_PIECE_WRITER_HPP
#define TORTOISE_PIECE_WRITER_HPP

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>

#include <tortoise/metainfo.hpp>

#include "../util/util.hpp"

namespace tortoise {
class PieceWriter final {
public:
  enum class State {
    Preparing, // Preparing files for writing, this includes reading existing files to determine which pieces are already downloaded
    Writing,   // Writing pieces to files
    Done,      // All pieces have been written, no more work to do
    Error      // An unrecoverable error has occured
  };

  /*!
   * \brief Creates a new PieceWriter.
   * \param metainfo The metainfo of the torrent.
   * \param output_path The path to the output directory.
   */
  PieceWriter(std::shared_ptr<const Metainfo> metainfo, const std::filesystem::path &output_path);
  ~PieceWriter();

  /*! \brief Writes a piece that was downloaded from a peer to the filesystem.
   *
   * \param piece_index The index of the piece to write.
   * \param data The data of the piece to write.
   */
  void WritePiece(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data);

  State GetState() const;

private:
  void Run(std::stop_token token);
  void Error(const std::string &error);
  void Flush();
  void Stop();

  void SetState(State state);
  void State_Prepare(const std::stop_token &stop_token);

  void State_Write(const std::stop_token &stop_token);

  const std::shared_ptr<const Metainfo> metainfo_;
  const std::filesystem::path output_path_;

  mutable std::mutex state_mutex_;
  State state_;

  struct Piece {
    std::uint32_t index;
    std::shared_ptr<const ByteVector> data;
  };
  std::condition_variable pieces_cv_;
  std::mutex pieces_mutex_;
  std::queue<Piece> pieces_queue_;

  struct FileInfo {
    std::ofstream *stream;
    std::filesystem::path path;
  };
  bool PrepareFileStreams(const std::stop_token &stop_token, std::vector<std::shared_ptr<FileInfo>> &filehandles);
  bool PrepareFilePieces(const std::stop_token &stop_token, const std::vector<std::shared_ptr<FileInfo>> &filehandles);
  bool ReadExistingFiles(const std::stop_token &stop_token);

  struct FilePiece {
    std::shared_ptr<FileInfo> file;
    std::size_t offset;
    std::size_t length;
  };
  std::vector<std::ofstream> filestreams_;
  std::vector<std::vector<FilePiece>> piece_to_file_;
  std::size_t pieces_done_;

  std::jthread thread_;
};
} // namespace tortoise

#endif