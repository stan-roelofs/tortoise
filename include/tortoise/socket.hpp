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

        /*! \brief Sets the socket to blocking or non-blocking mode.
         *  \param blocking True to set the socket to blocking mode, false to set it to non-blocking mode.
         *  \return True on success, false on error.
         */
        bool SetBlocking(bool blocking);

        //! \returns True if the socket is in blocking mode, false if it is in non-blocking mode.
        bool GetBlocking() const;

        //! \returns True if the socket is connected, false otherwise.
        bool Connected() const;

    private:
        bool SetBlockingInternal(bool blocking);

#ifdef _WIN32
        using socket_t = SOCKET;
        static constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
        using socket_t = int;
        static constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

        socket_t socket_;
        bool blocking_;
    };
} // namespace tortoise

#endif