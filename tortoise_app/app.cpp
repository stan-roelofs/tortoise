#include "app.hpp"

#include <assert.h>
#include <iostream>

#include <curses.h>

#include <tortoise/load_torrent.hpp>
#include <tortoise/metainfo.hpp>

namespace
{
    constexpr int WINDOW_HEIGHT = 10;
}

struct Window
{
    Window(const std::string &t, int y) : title(t), window(nullptr), connecting(0), connected(0)
    {
        const int width = getmaxx(stdscr);

        window = newwin(WINDOW_HEIGHT, width, y, 0);
        Repaint();
    }

    void PeerConnecting(const std::string &ip, std::uint16_t port)
    {
        std::string peer = ip + std::to_string(port);
        const auto it = peers.insert(std::make_pair(peer, tortoise::PeerStatus::Connecting));
        if (it.second)
        {
            ++connecting;
            UpdatePeers();
        }
    }

    void PeerConnected(const std::string &ip, std::uint16_t port)
    {
        std::string peer = ip + std::to_string(port);
        const auto it = peers.find(peer);
        assert(it != peers.end());
        if (it == peers.end())
            return;

        it->second = tortoise::PeerStatus::Connected;
        ++connected;
        --connecting;
        UpdatePeers();
    }

    void PeerDisconnected(const std::string &ip, std::uint16_t port)
    {
        std::string peer = ip + std::to_string(port);
        const auto it = peers.find(peer);
        assert(it != peers.end());
        if (it == peers.end())
            return;

        switch (it->second)
        {
        case tortoise::PeerStatus::Connecting:
            --connecting;
            break;
        case tortoise::PeerStatus::Connected:
            --connected;
            break;
        }

        peers.erase(it);
        UpdatePeers();
    }

    void UpdatePeers()
    {
        wmove(window, 2, 2);

        wprintw(window, "Connecting: %d / Connected: %d       ", connecting, connected);
        Refresh();
    }

    void Resize(int width, int height)
    {
        wresize(window, height, width);
        Repaint();
    }

    void Repaint()
    {
        wclear(window);

        box(window, 0, 0);
        mvwaddstr(window, 1, 2, title.c_str());
        UpdatePeers();
        Refresh();
    }

    void Refresh()
    {
        if (!isendwin())
            wrefresh(window);
    }

    std::string title;
    WINDOW *window;

    std::map<std::string, tortoise::PeerStatus> peers;
    int connecting;
    int connected;
};

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
    windows_.insert(std::make_pair(torrent, new Window(torrent.GetMetainfo().name, (int)windows_.size() * WINDOW_HEIGHT)));
}

void Application::OnPeerStatusChanged(tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status)
{
    switch (status)
    {
    case tortoise::PeerStatus::Connecting:
        windows_[torrent]->PeerConnecting(ip, port);
        break;
    case tortoise::PeerStatus::Connected:
        windows_[torrent]->PeerConnected(ip, port);
        break;
    case tortoise::PeerStatus::Disconnected:
        windows_[torrent]->PeerDisconnected(ip, port);
        break;
    }
}

int Application::Run()
{
    if (!AddTorrent(args_.torrent_file))
        return EXIT_FAILURE;

    if (args_.curses)
        return RunCurses();
    else
        return RunConsole();
}

int Application::RunCurses()
{
    initscr();
    cbreak();
    noecho();
    curs_set(0);
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
                for (auto &window : windows_)
                    window.second->Resize(getmaxx(stdscr), WINDOW_HEIGHT);
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

int Application::RunConsole()
{
    bool quit = false;
    while (!quit)
    {
        // session_->HandleEvents();

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    return EXIT_SUCCESS;
}