#include "app.hpp"

#include <assert.h>
#include <format>
#include <iostream>

#include <tortoise/load_torrent.hpp>
#include <tortoise/logging.hpp>
#include <tortoise/metainfo.hpp>

Application::Application(CommandLineArguments args) : args_(args)
{
    tortoise::logging::RegisterReceiver([](const tortoise::logging::Message& message)
        {
            std::cout << std::format("{:%T} ", message.time);

                                            switch(message.level)
                                            {
                                                case tortoise::logging::Level::Debug:
                                                    std::cout << "[DEBUG] ";
                                                    break;
                                                case tortoise::logging::Level::Info:
                                                    std::cout << "[INFO] ";
                                                    break;
                                                case tortoise::logging::Level::Warning:
                                                    std::cout << "[WARNING] ";
                                                    break;
                                                case tortoise::logging::Level::Error:
                                                    std::cout << "[ERROR] ";
                                                    break;
                                            }


                                            std::cout << std::format("{} : {}", message.tag, message.message) << std::endl; });

    tortoise::EventCallbacks event_callbacks;
    event_callbacks.torrent_added = std::bind(&Application::OnTorrentAdded, this, std::placeholders::_1);
    event_callbacks.peer_status_changed = std::bind(&Application::OnPeerStatusChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

    session_ = std::make_unique<tortoise::Session>(event_callbacks);
}

bool Application::AddTorrent(const std::string &torrent_file)
{
    auto metainfo = tortoise::LoadTorrent(torrent_file);
    if (!metainfo)
    {
        std::cout << "Failed to load torrent file: " << torrent_file << std::endl;
        return false;
    }

    tortoise::TorrentParameters torrent_params(*metainfo);
    torrent_params.save_path = ".";
    auto handle = session_->AddTorrent(torrent_params);
    if (!handle)
    {
        std::cout << "Failed to add torrent" << std::endl;
        return false;
    }

    handle.StartDownload();

    torrents_.push_back(handle);

    return true;
}

void Application::OnTorrentAdded(tortoise::TorrentHandle torrent)
{
    std::cout << "Added torrent: " << torrent.GetMetainfo().name << std::endl;
}

void Application::OnPeerStatusChanged(tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status)
{
    switch (status)
    {
    case tortoise::PeerStatus::Connecting:
        std::cout << "Connecting to peer " << ip << ":" << port << std::endl;
        break;
    case tortoise::PeerStatus::Connected:
        std::cout << "Connected to peer " << ip << ":" << port << std::endl;
        break;
    case tortoise::PeerStatus::Disconnected:
        std::cout << "Disconnected from peer " << ip << ":" << port << std::endl;
        break;
    }
}

int Application::Run()
{
    if (!AddTorrent(args_.torrent_file))
        return EXIT_FAILURE;

    running_ = true;
    while (running_)
    {
        session_->HandleEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}