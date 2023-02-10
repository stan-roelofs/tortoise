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