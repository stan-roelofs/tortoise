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
	bool AddTorrent(const std::string &torrent_file);

	void OnTorrentStatusChanged(const tortoise::event::TorrentStatusChanged &event);
	void OnTorrentError(const tortoise::event::TorrentError &event);
	void OnPeerStatusChanged(const tortoise::event::PeerStatusChanged &event);
	void OnPieceDownloaded(const tortoise::event::PieceDownloaded &event);

	CommandLineArguments args_;
	std::unique_ptr<tortoise::Session> session_;

	struct TorrentInfo
	{
		TorrentInfo(std::size_t pieces) : pieces_downloaded(0)
		{
			have_pieces.resize(pieces);
		}
		std::map<tortoise::PeerInfo, tortoise::PeerStatus> peers;
		std::vector<bool> have_pieces;
		std::uint32_t pieces_downloaded;
	};
	tortoise::TorrentHandle handle_;
	std::unique_ptr<TorrentInfo> data_;
};

#endif