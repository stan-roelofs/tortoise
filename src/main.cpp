#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <tortoise/metainfo.hpp>
#include <tortoise/tracker.hpp>

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

  std::ifstream file(argv[arg], std::ios::binary);
  if (!file.is_open())
  {
    std::cout << "Failed to open file: " << argv[1] << std::endl;
    return EXIT_FAILURE;
  }
  
  using namespace tortoise;
  std::unique_ptr<bencode::Data> data;
  try
  {
    data = bencode::Decode(file);
  }
  catch (const BencodeException &e)
  {
    std::cout << "Failed to decode bencode: " << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  auto metainfo = Metainfo::FromBencode(*data);
  Tracker tracker(*metainfo);
  tracker.Announce();
}