#ifndef TORTOISE_CONNECTION_HPP
#define TORTOISE_CONNECTION_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "socket.hpp"

namespace tortoise
{
    class Connection
    {
    public:
        /*! \brief Creates a new outgoing connection to the specified host and port.
         *  \param host The host to connect to.
         *  \param port The port to connect to.
         *  \return A pointer to the new connection. This may be null if the connection could not be established.
         */
        static std::unique_ptr<Connection> CreateOutgoing(const std::string &host, const std::string &port);

        /*! \brief Creates a new incoming connection on the specified port.
         *  \param port The port to listen on.
         *  \return A pointer to the new connection. This may be null if the connection could not be established.
         */
        static std::unique_ptr<Connection> CreateIncoming(const std::string &port);

        ~Connection();

        /*! \brief Sends data to the connected host.
         *  \param data The data to send.
         *  \param size The size of the data to send.
         *  \return The number of bytes that was actually sent. This may be less than the size of the data. -1 is returned on error.
         */
        int Send(const void *data, int size);

        /*! \brief Receives data from the connected host.
         *  \param data The buffer to store the received data in.
         *  \param size The size of the buffer.
         *  \return The number of bytes that was actually received. This may be less than the size of the buffer. -1 is returned on error.
         */
        int Receive(void *data, int size);

        /*! \brief Sends data to the connected host, blocks until all data has been sent.
         *  \param data The data to send.
         *  \param size The size of the data to send.
         *  \return True if all data was sent. False if an error occurred.
         */
        bool SendAll(const void *data, int size);

        /*! \brief Receives all data from the connected host.
         *  \param buffer The buffer to store the received data in.
         *  \return True if all data was received successfully. False if an error occurred.
         */
        bool ReceiveAll(std::vector<std::uint8_t> &buffer);

    private:
        Connection(std::unique_ptr<Socket> socket);

        std::unique_ptr<Socket> socket_;
    };
} // namespace tortoise

#endif