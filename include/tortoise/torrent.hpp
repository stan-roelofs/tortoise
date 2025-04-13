#ifndef TORTOISE_TORRENT_HPP
#define TORTOISE_TORRENT_HPP

#include <filesystem>
#include <memory>
#include <string>

#include "exception.hpp"
#include "metainfo.hpp"

namespace tortoise
{
	class Torrent;

	enum class TorrentStatus
	{
		Downloading,
		Seeding,
		Stopped
	};

	struct Statistics
	{
		/// Upload rate in bytes per second
		std::uint64_t upload_rate = 0;
		/// Download rate in bytes per second
		std::uint64_t download_rate = 0;
	};

	//! \brief A non-owning handle to a torrent.
	class TorrentHandle
	{
	public:
		TorrentHandle(std::weak_ptr<Torrent> ptr) : ptr_(std::move(ptr)) {}

		/*! \returns Returns the metainfo of the torrent.
		 *  \throws InvalidHandleException if the torrent is not valid.
		 */
		std::shared_ptr<const Metainfo> GetMetainfo() const;

		/*! \brief Starts downloading the torrent.
		 *  \return True if the torrent was started successfully.
		 *  \throws InvalidHandleException if the torrent is not valid.
		 */
		bool StartDownload();

		/*! \brief Stops downloading the torrent.
		 *  \throws InvalidHandleException if the torrent is not valid.
		 */
		void StopDownload();

		//! \brief Returns the current statistics of the torrent
		Statistics GetStatistics() const;

		/*! \brief Returns the status of the torrent.
		 *  \throws InvalidHandleException if the torrent is not valid.
		 */
		TorrentStatus GetStatus() const;

		bool IsValid() const;
		operator bool() const;
		bool operator==(const TorrentHandle &other) const;
		bool operator!=(const TorrentHandle &other) const;

	private:
		std::weak_ptr<Torrent> ptr_;
	};
} // namespace tortoise

#endif