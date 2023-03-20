#ifndef TORTOISE_SESSION_HPP
#define TORTOISE_SESSION_HPP

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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
            bool start = true; // If true, the session will start immediately. Otherwise, the session will be stopped until Start() is called.
        };

        Session(Parameters parameters);
        ~Session();

        /*! \brief Adds a torrent to the session.
         *  \param torrent Parameters for the the new torrent.
         *  \return A handle to the torrent.
         */
        Torrent::Handle AddTorrent(const Torrent::Parameters &parameters);

        /*! \brief Removes a torrent from the session. If the torrent is not in the session, this function has no effect.
         *  \param handle handle to the torrent.
         */
        void RemoveTorrent(Torrent::Handle handle);

        //! \brief Starts the session.
        void Start();

        //! \brief Stops the session.
        void Stop();

        // Disable copy and move.
        Session(const Session &) = delete;
        Session(Session &&) = delete;
        Session &operator=(const Session &) = delete;
        Session &operator=(Session &&) = delete;

    private:
        static void ThreadFunc(Session &session);
        void Run();

        bool running_ = true;
        std::vector<std::shared_ptr<Torrent>> torrents_;
        std::thread thread_;
        std::mutex mutex_;
    };
}

#endif