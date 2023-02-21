#ifndef TORTOISE_SESSION_HPP
#define TORTOISE_SESSION_HPP

#include <memory>
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
    class Session final
    {
    public:
        Session();
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

        // Disable copy and move.
        Session(const Session &) = delete;
        Session(Session &&) = delete;
        Session &operator=(const Session &) = delete;
        Session &operator=(Session &&) = delete;

    private:
        std::vector<std::shared_ptr<Torrent>> torrents_;
    };
}

#endif