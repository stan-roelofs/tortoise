#include "tiered_list.hpp"

#include <algorithm>
#include <cassert>
#include <random>

#include <tortoise/exceptions.hpp>

namespace tortoise
{
    namespace tracker
    {
        TieredList::TieredList(const std::vector<std::vector<std::string>> &trackers)
        {
            if (trackers.empty())
                throw InvalidArgumentException("trackers is empty");

            auto rd = std::random_device{};
            auto rng = std::default_random_engine{rd()};

            for (const auto &tier : trackers)
            {
                if (tier.empty())
                    continue;

                std::vector<std::string> tier_trackers(tier.begin(), tier.end());

                // URLs within each tier will be processed in a randomly chosen order; in other words, the list will be shuffled when first read.
                if (tier_trackers.size() > 1)
                    std::shuffle(tier_trackers.begin(), tier_trackers.end(), rng);

                std::list<std::string> tier_list(tier_trackers.begin(), tier_trackers.end());
                trackers_.emplace_back(tier_list);
            }

            SelectFirstTracker();
        }

        std::string TieredList::GetCurrentTracker() const
        {
            if (current_tier_ == trackers_.end() || current_tracker_ == current_tier_->end())
                return {};
            return *current_tracker_;
        }

        std::string TieredList::SelectNextTracker()
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

        void TieredList::PromoteCurrentTracker()
        {
            if (current_tier_ == trackers_.end() || current_tracker_ == current_tier_->end() || current_tracker_ == current_tier_->begin())
                return;

            auto current_tracker_copy = current_tracker_;
            --current_tracker_;
            std::swap(*current_tracker_, *(current_tracker_copy));
        }

        void TieredList::SelectFirstTracker()
        {
            current_tier_ = trackers_.begin();
            current_tracker_ = current_tier_->begin();
        }

        void TieredList::RemoveCurrentTracker()
        {
            if (current_tier_ == trackers_.end() || current_tracker_ == current_tier_->end())
                return;

            auto it = current_tier_->erase(current_tracker_);
            if (it == current_tier_->end())
            {
                ++current_tier_;
                current_tracker_ = current_tier_->begin();
            }
            else
                current_tracker_ = it;
        }
    }
}