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

        /*!
         * \brief Creates a peer id from a string.
         * \param peer_id The 20-byte peer id as a string.
         * \returns The peer id.
         * \throws InvalidArgumentException If the string is not 20 bytes long.
         */
        static PeerId FromString(const std::string &peer_id);

        //! \brief Returns the peer id as a string.
        const std::string &Get() const;

		bool operator==(const PeerId& other) const;
		bool operator!=(const PeerId& other) const;

    private:
        PeerId(const std::string &peer_id);
        std::string peer_id_;
    };
} // namespace tortoise

#endif