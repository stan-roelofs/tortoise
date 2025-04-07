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
		struct TorrentError
		{
			enum class Code
			{
				FileError,
			};

			TorrentHandle handle;
			Code code;
		};

		struct TorrentStatusChanged
		{
			TorrentHandle handle;
			TorrentStatus status;
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
			std::function<void(TorrentStatusChanged)> torrent_status_changed;
			std::function<void(TorrentError)> torrent_error;
			std::function<void(PeerStatusChanged)> peer_status_changed;
			std::function<void(PieceDownloaded)> piece_downloaded;
		};
	}

} // namespace tortoise

#endif