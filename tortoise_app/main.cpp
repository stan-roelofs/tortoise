#include <fstream>
#include <iostream>
#include <string>
#include <vector>

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
  tortoise::Session session(params);
  auto metainfo = tortoise::LoadTorrentFile(argv[arg]);
  if (!metainfo)
  {
    std::cout << "Failed to load torrent file: " << argv[arg] << std::endl;
    return EXIT_FAILURE;
  }

  tortoise::Torrent::Parameters torrent_params(*metainfo, tortoise::PeerId());
  torrent_params.save_path = ".";
  auto handle = session.AddTorrent(torrent_params);
  if (!handle)
  {
    std::cout << "Failed to add torrent" << std::endl;
    return EXIT_FAILURE;
  }

  while (true)
  {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
}