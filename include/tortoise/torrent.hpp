#ifndef TORTOISE_TORRENT_HPP
#define TORTOISE_TORRENT_HPP

#include <memory>
#include <string>

#include "metainfo.hpp"
#include "peer_id.hpp"
#include "tracker_announce.hpp"
#include "tracker_manager.hpp"

namespace tortoise
{
    class Torrent
    {
    public:
        //! \brief A non-owning handle to a torrent.
        class Handle
        {
        public:
            Handle(const std::shared_ptr<const Torrent> &ptr) : ptr_(ptr) {}

            bool IsValid() const
            {
                return !ptr_.expired();
            }

            operator bool() const
            {
                return IsValid();
            }

            bool operator==(const Handle &other) const
            {
                return ptr_.lock() == other.ptr_.lock();
            }

            bool operator!=(const Handle &other) const
            {
                return !(*this == other);
            }

        private:
            std::weak_ptr<const Torrent> ptr_;
        };

        class TorrentException : public Exception
        {
        public:
            TorrentException(const std::string &msg) : Exception(msg) {}
        };

        struct Parameters
        {
            Parameters(const Metainfo &info, const PeerId &id) : metainfo(info), peer_id(id) {}
            Metainfo metainfo;
            std::string save_path;
            PeerId peer_id;
        };

        Torrent(const Parameters &params);
        ~Torrent();

        //! \brief This should be called periodically and will do all the work that is required to keep the torrent running.
        void Process();

    private:
        AnnounceParameters GetTrackerRequest();

        Parameters parameters_;
        TrackerManager tracker_manager_;
    };
} // namespace tortoise

#endif