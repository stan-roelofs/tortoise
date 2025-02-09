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

    void OnTorrentAdded(tortoise::TorrentHandle torrent);
    void OnPeerStatusChanged(tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status);

    CommandLineArguments args_;
    std::unique_ptr<tortoise::Session> session_;
    std::vector<tortoise::TorrentHandle> torrents_;
    std::atomic_bool running_;
};

#endif