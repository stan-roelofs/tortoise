#include <tortoise/exceptions.hpp>
#include <tortoise/socket.hpp>

namespace tortoise
{
    Socket::Socket() : socket_(INVALID_SOCKET_VALUE)
    {
#ifdef _WIN32

        WSADATA wsaData;
        int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
        if (iResult != 0)
            throw SocketException("WSAStartup failed: " + std::to_string(iResult));
#endif
    }

    Socket::~Socket()
    {
#ifdef _WIN32
        WSACleanup();
#endif
        Close();
    }

    void Socket::Close()
    {
#ifdef _WIN32
        closesocket(socket_);
#else
        close(socket_);
#endif
        socket_ = INVALID_SOCKET_VALUE;
    }

    bool Socket::Connect(const std::string &host, const std::string &port)
    {
        struct addrinfo *result = nullptr, hints;

        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = 0;

        int iResult = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
        if (iResult != 0)
            return false;

        for (addrinfo *ptr = result; ptr != nullptr; ptr = ptr->ai_next)
        {
            socket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (socket_ == INVALID_SOCKET_VALUE)
                continue;

            iResult = connect(socket_, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (iResult == 0)
                break;

            Close();
        }

        freeaddrinfo(result);

        if (socket_ == INVALID_SOCKET_VALUE)
            return false;

        return true;
    }

    bool Socket::Listen(const std::string &port)
    {
        // TODO
        (void)port;
        return false;
    }

    int Socket::Send(const void *data, int size)
    {
        int iResult = send(socket_, (const char *)data, size, 0);
        if (iResult == SOCKET_ERROR)
            return -1;

        return iResult;
    }

    int Socket::Receive(void *buffer, int size)
    {
        int iResult = recv(socket_, static_cast<char *>(buffer), size, 0);
        if (iResult == SOCKET_ERROR)
            return -1;

        return iResult;
    }

} // namespace tortoise