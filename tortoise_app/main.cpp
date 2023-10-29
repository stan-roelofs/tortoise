#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <curses.h>

#include <tortoise/load_torrent.hpp>
#include <tortoise/metainfo.hpp>
#include <tortoise/session.hpp>
#include <tortoise/torrent.hpp>

static int usage(const char *argv[])
{
  std::cout << "Usage: " << argv[0] << " [-h?] <file>*" << std::endl;
  std::cout << "  -h, -?  Show this help" << std::endl;
  return EXIT_FAILURE;
}

int main(int argc, const char *argv[])
{
  if (argc < 2)
    return usage(argv);

  int arg = 1;
  while (arg < argc && argv[arg][0] == '-')
  {
    switch (argv[arg][1])
    {
    case 'h':
    case '?':
      return usage(argv);
      break;
    default:
      std::cout << "Unknown option: " << argv[arg] << std::endl;
      return usage(argv);
    }
  }

  if (arg >= argc)
  {
    std::cout << "No file(s) specified" << std::endl;
    return usage(argv);
  }

  tortoise::Session::Parameters params;
  params.event_mask.set(); // Set all events

  tortoise::Session session(params);
  auto metainfo = tortoise::LoadTorrent(argv[arg]);
  if (!metainfo)
  {
    std::cout << "Failed to load torrent file: " << argv[arg] << std::endl;
    return EXIT_FAILURE;
  }

  tortoise::TorrentParameters torrent_params(*metainfo);
  torrent_params.save_path = ".";
  auto handle = session.AddTorrent(torrent_params);
  if (!handle)
  {
    std::cout << "Failed to add torrent" << std::endl;
    return EXIT_FAILURE;
  }

  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);
  keypad(stdscr, TRUE);

  tortoise::EventCallbacks callbacks;
  callbacks.torrent_added = [](tortoise::TorrentHandle torrent)
  {
    waddstr(stdscr, "Torrent added\n");
  };
  callbacks.peer_status_changed = [](tortoise::TorrentHandle torrent, const std::string &ip, std::uint16_t port, tortoise::PeerStatus status)
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
  };

  int ch;
  bool quit = false;
  while (!quit)
  {
    if ((ch = getch()) != ERR)
    {
      switch (ch)
      {
      case 'q':
        quit = true;
        break;
      }
    }

    session.PopEvents(callbacks);
    wrefresh(stdscr);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  endwin();

  return EXIT_SUCCESS;
}