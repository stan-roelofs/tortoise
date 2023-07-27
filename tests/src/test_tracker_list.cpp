#include <gtest/gtest.h>

#include <algorithm>

#include "tracker_list.hpp"

TEST(TrackerList, initial_tracker)
{
    const std::vector<std::vector<std::string>> trackers = {{"test1", "test2", "test3"}, {"test4", "test5", "test6"}, {"test7", "test8", "test9"}};
    tortoise::TrackerList tracker_list(trackers);

    const auto &first_tier = trackers.at(0);
    EXPECT_TRUE(std::find(first_tier.begin(), first_tier.end(), tracker_list.GetCurrentTracker()) != first_tier.end());
}

TEST(TrackerList, select_next_tracker)
{
    std::vector<std::vector<std::string>> trackers = {{"test1", "test2", "test3"}, {"test4", "test5", "test6"}, {"test7", "test8", "test9"}};
    tortoise::TrackerList tracker_list(trackers);

    auto &first_tier = trackers.at(0);
    auto &second_tier = trackers.at(1);
    auto &third_tier = trackers.at(2);

    const auto check_tier = [&tracker_list](std::vector<std::string> tier)
    {
        while (!tier.empty())
        {
            const auto find_result = std::find(tier.begin(), tier.end(), tracker_list.GetCurrentTracker());
            ASSERT_TRUE(find_result != tier.end());
            tier.erase(find_result);
            tracker_list.SelectNextTracker();
        }
    };

    for (const auto &tier : trackers)
        check_tier(tier);
}