#include <tortoise/exceptions.hpp>
#include <tortoise/socket.hpp>

namespace tortoise
{
    Socket::Socket() : socket_(INVALID_SOCKET_VALUE), blocking_(true)
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

            if (!SetBlockingInternal(blocking_))
                return false;

            if (connect(socket_, ptr->ai_addr, (int)ptr->ai_addrlen) == 0)
                break;

            if (!GetBlocking())
            {
#ifdef _WIN32
                int error = WSAGetLastError();
                if (error == WSAEWOULDBLOCK)
                    break;
#else
                if (errno == EINPROGRESS)
                    break;
#endif
            }

            Close();
        }

        freeaddrinfo(result);

        return socket_ != INVALID_SOCKET_VALUE;
    }

    bool Socket::Listen(const std::string &port)
    {
        // TODO
        (void)port;
        return false;
    }

    int Socket::Send(const void *data, int size)
    {
        return send(socket_, (const char *)data, size, 0);
    }

    int Socket::Receive(void *buffer, int size)
    {
        return recv(socket_, static_cast<char *>(buffer), size, 0);
    }

    bool Socket::SetBlocking(bool blocking)
    {
        blocking_ = blocking;

        if (socket_ == INVALID_SOCKET_VALUE)
            return true;

        return SetBlockingInternal(blocking);
    }

    bool Socket::GetBlocking() const
    {
        return blocking_;
    }

    bool Socket::Connected() const
    {
        if (socket_ == INVALID_SOCKET_VALUE)
            return false;

        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0)
            return false;

        return error == 0;
    }

    bool Socket::SetBlockingInternal(bool blocking)
    {

#ifdef _WIN32
        u_long mode = blocking ? 0 : 1;
        return ioctlsocket(socket_, FIONBIO, &mode) == 0;
#else
        int flags = fcntl(socket_, F_GETFL, 0);
        if (flags == -1)
            return;

        if (blocking)
            flags &= ~O_NONBLOCK;
        else
            flags |= O_NONBLOCK;

        return fcntl(socket_, F_SETFL, flags) == 0;
#endif
    }
} // namespace tortoise