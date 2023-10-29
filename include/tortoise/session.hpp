#ifndef TORTOISE_SESSION_HPP
#define TORTOISE_SESSION_HPP

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "event.hpp"
#include "torrent.hpp"

namespace tortoise
{
    /*! \brief A session manages a collection of torrents.
     *
     * The session is responsible for managing the network connections,
     * disk access, and other resources used by the torrents.
     * The session will spawn a thread that will do all the work.
     */
    class Session
    {
    public:
        struct Parameters
        {
            bool start = true;       // If true, the session will start immediately. Otherwise, the session will be stopped until Start() is called.
            event_bitset event_mask; // Determines which events will be generated.
        };

        Session(Parameters parameters);
        ~Session();

        /*! \brief Adds a torrent to the session.
         *  \param torrent Parameters for the the new torrent.
         *  \return A handle to the torrent.
         */
        TorrentHandle AddTorrent(const TorrentParameters &parameters);

        /*! \brief Removes a torrent from the session. If the torrent is not in the session, this function has no effect.
         *  \param handle handle to the torrent.
         */
        void RemoveTorrent(TorrentHandle handle);

        //! \brief Starts the session.
        void Start();

        //! \brief Stops the session.
        void Stop();

        /*! \brief Pop all events and process them using the given callbacks. Note that only events that are enabled in the session will be generated.
         *  \param callbacks A struct containing callbacks for each event.
         */
        void PopEvents(const EventCallbacks &callbacks);

        // Disable copy and move.
        Session(const Session &) = delete;
        Session(Session &&) = delete;
        Session &operator=(const Session &) = delete;
        Session &operator=(Session &&) = delete;

    private:
        class Implementation;
        std::unique_ptr<Implementation> implementation_;
    };
}

#endif