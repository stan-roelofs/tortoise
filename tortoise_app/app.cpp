#include "app.hpp"

#include <iostream>

#include <curses.h>

#include <tortoise/load_torrent.hpp>
#include <tortoise/metainfo.hpp>

Application::Application(CommandLineArguments args) : args_(args)
{
    tortoise::Session::Parameters params;
    params.callbacks.torrent_added = std::bind(&Application::OnTorrentAdded, this, std::placeholders::_1);
    params.callbacks.peer_status_changed = std::bind(&Application::OnPeerStatusChanged, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

    session_ = std::make_unique<tortoise::Session>(params);
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

    torrents_.push_back(handle);

    return true;
}

void Application::OnTorrentAdded(tortoise::TorrentHandle torrent)
{
    addstr(std::string("Torrent added: " + torrent.GetMetainfo().name + "\n").c_str());
}

void Application::OnPeerStatusChanged(tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status)
{
    switch (status)
    {
    case tortoise::PeerStatus::Connecting:
        waddstr(stdscr, std::string("Peer connecting: " + ip + ":" + std::to_string(port) + "\n").c_str());
        break;
    case tortoise::PeerStatus::Connected:
        waddstr(stdscr, std::string("Peer connected: " + ip + ":" + std::to_string(port) + "\n").c_str());
        break;
    case tortoise::PeerStatus::Disconnected:
        waddstr(stdscr, std::string("Peer disconnected: " + ip + ":" + std::to_string(port) + "\n").c_str());
        break;
    }
}

int Application::Run()
{
    if (!AddTorrent(args_.torrent_file))
        return EXIT_FAILURE;

    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);
    keypad(stdscr, TRUE);
    refresh();

    int ch;
    bool quit = false;
    while (!quit)
    {
        if ((ch = getch()) != ERR)
        {
            switch (ch)
            {
            case KEY_RESIZE:
            {
                resize_term(0, 0);
                // TODO: Resize windows
                break;
            }

            case 'q':
                quit = true;
                break;
            }
        }

        session_->HandleEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    endwin();

    return EXIT_SUCCESS;
}