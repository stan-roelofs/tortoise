#ifndef TORTOISE_SOCKET_HPP
#define TORTOISE_SOCKET_HPP

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")

#else
// Assume unix
// TODO

#endif

#include <string>

namespace tortoise
{
    //! \brief A platform independent socket class.
    class Socket
    {
    public:
        Socket();
        ~Socket();

        //! \brief Connects to a host.
        bool Connect(const std::string &host, const std::string &port);

        //! \brief Listens on a port.
        bool Listen(const std::string &port);

        //! \brief Closes the socket.
        void Close();

        /*! \brief Sends data to the connected host.
         *  \param data The data to send.
         *  \param size The size of the data to send.
         *  \return The number of bytes that was actually sent. This may be less than the size of the data. -1 is returned on error.
         */
        int Send(const void *data, int size);

        /*! \brief Receives data from the connected host.
         *  \param buffer The buffer to store the received data in.
         *  \param size The size of the buffer.
         *  \return The number of bytes that was actually received. This may be less than the size of the buffer. -1 is returned on error.
         */
        int Receive(void *buffer, int size);

    private:
#ifdef _WIN32
        using socket_t = SOCKET;
        static constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
        using socket_t = int;
        static constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

        socket_t socket_;
    };
} // namespace tortoise

#endif