#ifndef TORTOISE_EVENT_HPP
#define TORTOISE_EVENT_HPP

#include <functional>
#include <string>

#include <tortoise/peer_info.hpp>
#include <tortoise/torrent.hpp>

namespace tortoise
{
	namespace event
	{
		struct TorrentAdded
		{
			TorrentHandle handle;
		};

		struct TorrentStarted
		{
			TorrentHandle handle;
		};

		struct TorrentStopped
		{
			TorrentHandle handle;
		};

		struct TorrentError
		{
			enum class Code
			{
				FileError,
			};

			TorrentHandle handle;
			Code code;
		};

		struct TorrentDownloaded
		{
			TorrentHandle handle;
		};

		struct PeerStatusChanged
		{
			TorrentHandle handle;
			PeerInfo info;
			PeerStatus status;
		};

		struct PieceDownloaded
		{
			TorrentHandle handle;
			std::uint32_t piece_index;
		};

		struct Callbacks
		{
			std::function<void(TorrentAdded)> torrent_added;
			std::function<void(TorrentStarted)> torrent_started;
			std::function<void(TorrentStopped)> torrent_stopped;
			std::function<void(TorrentDownloaded)> torrent_downloaded;
			std::function<void(TorrentError)> torrent_error;
			std::function<void(PeerStatusChanged)> peer_status_changed;
			std::function<void(PieceDownloaded)> piece_downloaded;
		};
	}

} // namespace tortoise

#endif