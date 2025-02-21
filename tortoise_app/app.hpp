#ifndef TORTOISE_APP_HPP
#define TORTOISE_APP_HPP

#include <filesystem>
#include <map>
#include <string>
#include <vector>

#include <tortoise/torrent.hpp>
#include <tortoise/session.hpp>

struct Window;

class Application
{
public:
	struct CommandLineArguments
	{
		std::string torrent_file;
	};

	Application(CommandLineArguments args);

	/*! \brief Run the application, blocking until it exits
	 *
	 * \return int Exit code
	 */
	int Run();

private:
	bool AddTorrent(const std::string& torrent_file);

	void OnTorrentAdded(const tortoise::event::TorrentAdded& event);
	void OnPeerStatusChanged(const tortoise::event::PeerStatusChanged& event);
	void OnPieceDownloaded(const tortoise::event::PieceDownloaded& event);

	CommandLineArguments args_;
	std::unique_ptr<tortoise::Session> session_;

	struct TorrentInfo
	{
		TorrentInfo(tortoise::TorrentHandle h) : handle(std::move(h)), pieces_downloaded(0) {
			have_pieces.resize(handle.GetMetainfo().pieces.size());
		}
		const tortoise::TorrentHandle handle;
		std::map<tortoise::PeerInfo, tortoise::PeerStatus> peers;
		std::vector<bool> have_pieces;
		std::uint32_t pieces_downloaded;
	};
	std::map<tortoise::TorrentHandle, TorrentInfo> torrents_;
	std::atomic_bool running_;
};

#endif