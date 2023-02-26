#ifndef TORTOISE_TORRENT_HPP
#define TORTOISE_TORRENT_HPP

#include <chrono>
#include <memory>
#include <string>

#include "metainfo.hpp"
#include "tracker_manager.hpp"

namespace tortoise
{
    class Torrent
    {
    public:
        //! \brief A non-owning handle to a torrent.
        struct Handle
        {
            Handle(const std::shared_ptr<const Torrent> &ptr) : ptr_(ptr) {}

            bool IsValid() const
            {
                return !ptr_.expired();
            }

            operator bool() const
            {
                return IsValid();
            }

        private:
            std::weak_ptr<const Torrent> ptr_;
        };

        struct Parameters
        {
            std::unique_ptr<Metainfo> metainfo;
            std::string save_path = "";
        };

        Torrent(const Parameters &params);
        ~Torrent();

        //! \brief This should be called periodically and will do all the work that is required to keep the torrent running.
        void Process();

    private:
        bool ShouldContractTracker() const;
        void ContactTracker();
        void HandleTrackerResponse(bool success);

        //! \brief The interval of the last tracker we contacted. Used to determine when to contact the tracker again. If 0, we have not contacted a tracker yet.
        std::uint64_t tracker_interval_;

        //! \brief The time of the last contact with the tracker.
        std::chrono::steady_clock::time_point last_tracker_contact_;
    };
} // namespace tortoise

#endif