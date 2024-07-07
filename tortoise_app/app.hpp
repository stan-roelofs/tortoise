#ifndef TORTOISE_APP_HPP
#define TORTOISE_APP_HPP

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
        bool curses = true;
        std::string torrent_file;
    };

    Application(CommandLineArguments args);

    int Run();

private:
    bool AddTorrent(const std::string &torrent_file);

    void OnTorrentAdded(tortoise::TorrentHandle torrent);
    void OnPeerStatusChanged(tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status);

    int RunCurses();
    int RunConsole();

    CommandLineArguments args_;
    std::unique_ptr<tortoise::Session> session_;
    std::vector<tortoise::TorrentHandle> torrents_;

    std::map<tortoise::TorrentHandle, Window *> windows_;
};

#endif