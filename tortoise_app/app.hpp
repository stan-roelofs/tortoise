#ifndef TORTOISE_APP_HPP
#define TORTOISE_APP_HPP

#include <string>

#include <tortoise/session.hpp>
#include <tortoise/torrent.hpp>

class Application
{
public:
    struct CommandLineArguments
    {
        std::string torrent_file;
    };

    Application(CommandLineArguments args);

    int Run();

private:
    bool AddTorrent(const std::string &torrent_file);

    void OnTorrentAdded(tortoise::TorrentHandle torrent);
    void OnPeerStatusChanged(tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status);

    CommandLineArguments args_;
    std::unique_ptr<tortoise::Session> session_;
    std::vector<tortoise::TorrentHandle> torrents_;
};

#endif