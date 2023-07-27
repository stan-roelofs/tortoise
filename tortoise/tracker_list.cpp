#include "tracker_list.hpp"

#include <algorithm>
#include <cassert>
#include <random>

#include <tortoise/exceptions.hpp>

namespace tortoise
{
    TrackerList::TrackerList(const std::vector<std::vector<std::string>> &trackers)
    {
        if (trackers.empty())
            throw InvalidArgumentException("trackers is empty");

        auto rd = std::random_device{};
        auto rng = std::default_random_engine{rd()};

        for (const auto &tier : trackers)
        {
            if (tier.empty())
                continue;

            std::vector<std::string> tier_trackers = tier;

            // URLs within each tier will be processed in a randomly chosen order; in other words, the list will be shuffled when first read.
            if (tier_trackers.size() > 1)
                std::shuffle(tier_trackers.begin(), tier_trackers.end(), rng);
            trackers_.emplace_back(tier_trackers);
        }

        SelectFirstTracker();
    }

    std::string TrackerList::GetCurrentTracker() const
    {
        return *current_tracker_;
    }

    std::string TrackerList::SelectNextTracker()
    {
        ++current_tracker_;
        if (current_tracker_ == current_tier_->end())
        {
            ++current_tier_;
            if (current_tier_ == trackers_.end())
                SelectFirstTracker();
            else
                current_tracker_ = current_tier_->begin();
        }

        return *current_tracker_;
    }

    void TrackerList::PromoteCurrentTracker()
    {
        if (current_tier_ == trackers_.end() || current_tracker_ == current_tier_->end() || current_tracker_ == current_tier_->begin())
            return;

        std::iter_swap(current_tracker_, current_tracker_ - 1);
    }

    void TrackerList::SelectFirstTracker()
    {
        current_tier_ = trackers_.begin();
        current_tracker_ = current_tier_->begin();
    }
}
