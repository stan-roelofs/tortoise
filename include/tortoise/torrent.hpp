#ifndef TORTOISE_TORRENT_HPP
#define TORTOISE_TORRENT_HPP

#include <memory>

#include "metainfo.hpp"

namespace tortoise
{
    class Torrent
    {
    public:
        //! \brief A non-owning handle to a torrent.
        struct Handle : std::weak_ptr<Torrent>
        {
            Handle(const std::shared_ptr<Torrent> &ptr) : std::weak_ptr<Torrent>(ptr) {}

            bool IsValid() const
            {
                return !expired();
            }

            operator bool() const
            {
                return IsValid();
            }
        };

        struct Parameters
        {
            Metainfo metainfo;
        };

        Torrent(const Parameters &params);
    };
} // namespace tortoise

#endif