#ifndef TORTOISE_TORRENT_HPP
#define TORTOISE_TORRENT_HPP

#include <memory>
#include <string>

#include "exception.hpp"
#include "metainfo.hpp"

namespace tortoise
{
    class Torrent;

    //! \brief A non-owning handle to a torrent.
    class TorrentHandle
    {
    public:
        TorrentHandle(const std::shared_ptr<const Torrent> &ptr) : ptr_(ptr) {}

        Metainfo GetMetainfo() const;

        bool IsValid() const;
        operator bool() const;
        bool operator==(const TorrentHandle &other) const;
        bool operator!=(const TorrentHandle &other) const;

    private:
        std::weak_ptr<const Torrent> ptr_;
    };

    class TorrentException : public Exception
    {
    public:
        TorrentException(const std::string &msg) : Exception(msg) {}
    };

    struct TorrentParameters
    {
        TorrentParameters(const Metainfo &info) : metainfo(info) {}
        Metainfo metainfo;
        std::string save_path;
    };
} // namespace tortoise

#endif