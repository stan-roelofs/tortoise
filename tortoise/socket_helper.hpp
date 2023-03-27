#ifndef TORTOISE_SOCKET_HELPER_HPP
#define TORTOISE_SOCKET_HELPER_HPP

#include <tortoise/socket.hpp>

#include <vector>

namespace tortoise
{
    namespace socket_helper
    {
        /*! \brief Sends all data to the socket and blocks until all data has been sent.
         *  \param socket The socket to send the data on.
         *  \param data The data to send.
         *  \param size The size of the data to send.
         *  \param timeout The timeout in milliseconds. 0 means no timeout.
         *  \return True if all data was sent. False if an error occurred.
         */
        bool SendAll(Socket &socket, const void *data, int size, unsigned int timeout = 0);

        /*! \brief Receives all data from the socket.
         *  \param socket The socket to receive the data from.
         *  \param buffer The buffer to store the received data in.
         *  \param timeout The timeout in milliseconds. 0 means no timeout.
         *  \return True if all data was received successfully. False if an error occurred.
         */
        bool ReceiveAll(Socket &socket, std::vector<std::uint8_t> &buffer, unsigned int timeout = 0);

        /*! \brief Receives a fixed amount of data from the socket.
         *  \param socket The socket to receive the data from.
         *  \param buffer The buffer to store the received data in.
         *  \param buffer_size The size of the buffer.
         *  \param timeout The timeout in milliseconds. 0 means no timeout.
         *  \return True if all data was received successfully. False if an error occurred.
         */
        bool ReceiveAll(Socket &socket, void *buffer, int buffer_size, unsigned int timeout);

        std::uint64_t ToNetworkByteOrder(std::uint64_t value);
        std::uint64_t FromNetworkByteOrder(std::uint64_t value);
        std::uint32_t ToNetworkByteOrder(std::uint32_t value);
        std::uint32_t FromNetworkByteOrder(std::uint32_t value);
        std::uint16_t ToNetworkByteOrder(std::uint16_t value);
        std::uint16_t FromNetworkByteOrder(std::uint16_t value);
    } // namespace socket_helper
}

#endif