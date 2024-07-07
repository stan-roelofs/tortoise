#ifndef TORTOISE_TORRENT_HPP
#define TORTOISE_TORRENT_HPP

#include <memory>
#include <string>

#include "exception.hpp"
#include "metainfo.hpp"

namespace tortoise
{
    class Torrent;

    class InvalidHandleException : public Exception
    {
    public:
        InvalidHandleException(const std::string &msg) : Exception(msg) {}
    };

    //! \brief A non-owning handle to a torrent.
    class TorrentHandle
    {
    public:
        TorrentHandle(const std::shared_ptr<Torrent> &ptr) : ptr_(ptr) {}

        /*! \returns Returns the metainfo of the torrent.
         *  \throws InvalidHandleException if the torrent is not valid.
         */
        Metainfo GetMetainfo() const;

        /*! \brief Starts downloading the torrent.
         *  \return True if the torrent was started successfully.
         *  \throws InvalidHandleException if the torrent is not valid.
         */
        void StartDownload();

        /*! \brief Stops downloading the torrent.
         *  \throws InvalidHandleException if the torrent is not valid.
         */
        void StopDownload();

        bool IsValid() const;
        operator bool() const;
        bool operator==(const TorrentHandle &other) const;
        bool operator!=(const TorrentHandle &other) const;

    private:
        std::weak_ptr<Torrent> ptr_;
    };

    struct TorrentParameters
    {
        TorrentParameters(const Metainfo &info) : metainfo(info) {}
        Metainfo metainfo;
        std::string save_path;
    };
} // namespace tortoise

#endif