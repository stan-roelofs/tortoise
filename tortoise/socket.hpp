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
#include <vector>

#include <tortoise/exception.hpp>

#include "network.hpp"

namespace tortoise
{
    //! \brief A platform independent socket class. All sockets are non-blocking!
    class Socket
    {
    public:
        enum class Result
        {
            Ok,
            WouldBlock,
            Error,
        };

        enum class Status
        {
            Connected,
            Pending,
            Error
        };

        class SocketException : public Exception
        {
        public:
            SocketException(const std::string &msg) : Exception(msg) {}
        };

        /*! \brief Creates a new socket.
         *  \param protocol The transport protocol to use.
         *  \throws SocketException if the socket could not be created.
         */
        Socket(network::TransportProtocol protocol);
        Socket(const Socket &) = delete;
        Socket(Socket &&) = delete;
        Socket &operator=(const Socket &) = delete;
        Socket &operator=(Socket &&) = delete;
        ~Socket();

        /*! \brief Connects to a host. If a connectionless protocol is used, this will merely set the default host and port for future send/receive calls.
         *  \param host The host to connect to.
         *  \param port The port to connect to.
         *  \return True if the connection was successful.
         */
        bool Connect(const std::string &host, const std::string &port);

        //! \brief Closes the socket.
        void Close();

        /*! \brief Sends data to the socket.
         *  \param data The data to send.
         *  \param size The size of the data to send. This will be updated to the number of bytes that was actually sent.
         *  \return Result::Ok if the data was sent successfully.
         *          Result::WouldBlock if the socket is non-blocking and the data could not be sent immediately.
         *          Result::Error if an error occurred.
         */
        Result Send(const void *data, int &length);

        /*! \brief Receives available data from the socket.
         *  \param buffer The buffer to store the received data in.
         *  \param length The maximum number of bytes to receive. This will be updated to the number of bytes that was actually written.
         *  \return Result::Ok if the data was received successfully.
         *          Result::WouldBlock if the socket is non-blocking and no data is available.
         *          Result::Error if an error occurred.
         */
        Result Receive(void *buffer, int &length);

        //! \returns The connection status
        Status GetConnectionStatus() const;

        //! \returns The transport protocol used by the socket.
        network::TransportProtocol GetTransportProtocol() const;

        //! \returns The internet protocol used by the socket.
        network::InternetProtocol GetInternetProtocol() const;

        static std::uint64_t ToNetworkByteOrder(std::uint64_t value);
        static std::uint64_t FromNetworkByteOrder(std::uint64_t value);
        static std::uint32_t ToNetworkByteOrder(std::uint32_t value);
        static std::uint32_t FromNetworkByteOrder(std::uint32_t value);
        static std::uint16_t ToNetworkByteOrder(std::uint16_t value);
        static std::uint16_t FromNetworkByteOrder(std::uint16_t value);

    private:
        bool SetBlocking(bool blocking);
        bool Select(bool read, bool write, unsigned int timeout_us) const;

#ifdef _WIN32
        using socket_t = SOCKET;
        static constexpr socket_t INVALID_SOCKET_VALUE = INVALID_SOCKET;
#else
        using socket_t = int;
        static constexpr socket_t INVALID_SOCKET_VALUE = -1;
#endif

        network::TransportProtocol protocol_;
        network::InternetProtocol internet_protocol_;
        socket_t socket_;
    };
} // namespace tortoise

#endif