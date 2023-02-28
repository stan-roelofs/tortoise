#ifndef TORTOISE_PEER_ID_HPP
#define TORTOISE_PEER_ID_HPP

#include <string>

namespace tortoise
{
    //! \brief A 20-byte unique identifier for the client.
    class PeerId
    {
    public:
        PeerId();
        ~PeerId();

        //! \brief Returns the peer id as a string.
        const std::string &Get() const;

    private:
        std::string peer_id_;
    };
} // namespace tortoise

#endif