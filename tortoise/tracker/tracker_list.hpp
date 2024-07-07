#ifndef TORTOISE_TRACKER_LIST_HPP
#define TORTOISE_TRACKER_LIST_HPP

#include <string>
#include <list>
#include <vector>

namespace tortoise
{
    //! \brief Stores a list of tiers of trackers, see BEP 12.
    class TrackerList
    {
    public:
        //! \throws InvalidArgumentException If the trackers list is empty.
        TrackerList(const std::vector<std::vector<std::string>> &trackers);

        //! \returns The current tracker URL.
        std::string GetCurrentTracker() const;

        /*! \brief If the current tracker could not be reached, this function selects the next tracker.
         *  \returns The next tracker URL.
         */
        std::string SelectNextTracker();

        //! \brief If a connection with the current tracker was succesful, this function moves it to the front of its tier.
        void PromoteCurrentTracker();

        //! \brief Select the first tracker in the first tier. This should be used after a successful announce.
        void SelectFirstTracker();

        //! \brief Removes the current tracker from the list.
        void RemoveCurrentTracker();

    private:
        std::list<std::list<std::string>> trackers_;
        std::list<std::list<std::string>>::iterator current_tier_;
        std::list<std::string>::iterator current_tracker_;
    };
}

#endif