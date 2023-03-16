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
    } // namespace socket_helper
}

#endif