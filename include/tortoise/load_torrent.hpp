#ifndef TORTOISE_LOAD_TORRENT_HPP
#define TORTOISE_LOAD_TORRENT_HPP

#include <memory>
#include <string>

#include "metainfo.hpp"

namespace tortoise
{
    std::unique_ptr<Metainfo> LoadTorrentFile(const std::string &path);
}

#endif