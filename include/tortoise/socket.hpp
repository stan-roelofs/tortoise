#ifndef TORTOISE_SOCKET_HPP
#define TORTOISE_SOCKET_HPP

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "Ws2_32.lib")
#else
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#endif

#include <string>

namespace tortoise
{
    //! \brief A platform independent socket class. All sockets are non-blocking!
    class Socket
    {
    public:
        Socket();
        ~Socket();

        /*! \brief Connects to a host.
         *  \param host The host to connect to.
         *  \param port The port to connect to.
         *  \param timeout_ms The timeout in milliseconds. 0 means no timeout and will return immediately.
         *  \return True if the connection was successful.
         */
        bool Connect(const std::string &host, const std::string &port, unsigned int timeout_ms);

        //! \brief Closes the socket.
        void Close();

        /*! \brief Sends data to the connected host.
         *  \param data The data to send.
         *  \param size The size of the data to send.
         *  \param timeout_ms The timeout in milliseconds. 0 means no timeout and will return immediately.
         *  \return The number of bytes that was actually sent. This may be less than the size of the data. -1 is returned on error.
         */
        int Send(const void *data, int size, unsigned int timeout_ms);

        /*! \brief Receives data from the connected host.
         *  \param buffer The buffer to store the received data in.
         *  \param size The size of the buffer.
         *  \param timeout_ms The timeout in milliseconds. 0 means no timeout and will return immediately.
         *  \return The number of bytes that was actually received. This may be less than the size of the buffer. -1 is returned on error.
         */
        int Receive(void *buffer, int size, unsigned int timeout_ms);

        //! \returns True if the socket is connected, false otherwise.
        bool Connected() const;

    private:
        bool SetBlocking(bool blocking);
        bool Select(bool read, bool write, unsigned int timeout_ms);

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