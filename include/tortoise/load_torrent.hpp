#ifndef TORTOISE_LOAD_TORRENT_HPP
#define TORTOISE_LOAD_TORRENT_HPP

#include <memory>
#include <string>

#include "metainfo.hpp"

namespace tortoise
{
    //! \brief Loads a torrent file from the given path.
    std::unique_ptr<Metainfo> LoadTorrent(const std::string &path);

    //! \brief Loads a torrent file from the given stream.
    std::unique_ptr<Metainfo> LoadTorrent(std::istream &stream);
}

#endif