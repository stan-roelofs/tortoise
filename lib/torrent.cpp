#include <tortoise/torrent.hpp>

#include <cassert>

namespace tortoise
{
    Torrent::Torrent(const Parameters &parameters) : tracker_interval_(0)
    {
        if (!parameters.metainfo)
            throw std::invalid_argument("metainfo is null");

        if (parameters.save_path.empty())
            throw std::invalid_argument("save_path is empty");

    }

    Torrent::~Torrent()
    {
    }

    void Torrent::Process()
    {
        if (ShouldContractTracker())
            ContactTracker();
    }

    bool Torrent::ShouldContractTracker() const
    {
        // No contact yet
        if (last_tracker_contact_ == std::chrono::steady_clock::time_point())
            return true;

        assert(tracker_interval_ > 0);

        const auto now = std::chrono::steady_clock::now();
        const auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - last_tracker_contact_);
        return static_cast<std::uint64_t>(elapsed.count()) > tracker_interval_;
    }

    void Torrent::ContactTracker()
    {
        last_tracker_contact_ = std::chrono::steady_clock::now();
        // TODO implement a class that manages tracker requests
    }

    void Torrent::HandleTrackerResponse(bool success)
    {
        // TODO
        if (!success)
        {
            last_tracker_contact_ = std::chrono::steady_clock::time_point();
        }
    }
}