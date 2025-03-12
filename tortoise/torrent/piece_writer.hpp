#ifndef TORTOISE_PIECE_WRITER_HPP
#define TORTOISE_PIECE_WRITER_HPP

#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <queue>
#include <stop_token>
#include <thread>
#include <mutex>

#include <tortoise/metainfo.hpp>

#include "../util/util.hpp"

namespace tortoise
{
	class PieceWriter final
	{
	public:
		struct Callbacks
		{
			std::function<void()> on_error;
			std::function<void()> on_done;
		};

		/*!
		 * \brief Creates a new PieceWriter.
		 * \param metainfo The metainfo of the torrent.
		 * \param output_path The path to the output directory.
		 */
		PieceWriter(const Metainfo &metainfo, std::filesystem::path output_path, Callbacks callbacks);
		~PieceWriter();

		/*! \brief Writes a piece that was downloaded from a peer to the filesystem.
		 *
		 * \param piece_index The index of the piece to write.
		 * \param data The data of the piece to write.
		 */
		void WritePiece(std::uint32_t piece_index, std::shared_ptr<const ByteVector> data);

	private:
		void Run(std::stop_token token);
		void Error(const std::string &error);
		void Flush();
		void Stop();

		Callbacks callbacks_;

		std::jthread thread_;
		std::condition_variable cv_;
		std::mutex mutex_;

		struct Piece
		{
			std::uint32_t index;
			std::shared_ptr<const ByteVector> data;
		};
		std::queue<Piece> pieces_queue_;

		struct FileInfo
		{
			std::ofstream *stream;
			std::filesystem::path path;
		};

		struct FilePiece
		{
			std::shared_ptr<FileInfo> file;
			std::size_t offset;
			std::size_t length;
		};
		std::vector<std::ofstream> filestreams_;
		std::vector<std::vector<FilePiece>> piece_to_file_;
		std::size_t pieces_done_;

		std::filesystem::path output_path_;
	};
} // namespace tortoise

#endif