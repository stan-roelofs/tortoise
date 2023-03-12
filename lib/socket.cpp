#include <tortoise/exceptions.hpp>
#include <tortoise/socket.hpp>

#include <cassert>

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

    bool Socket::Connect(const std::string &host, const std::string &port, unsigned int timeout)
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

            if (!SetBlocking(false))
                return false;

            if (connect(socket_, ptr->ai_addr, (int)ptr->ai_addrlen) == 0)
                break;

            if (timeout == 0)
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
            else
            {
                if (Select(false, true, timeout))
                    break;
            }

            Close();
        }

        freeaddrinfo(result);

        return socket_ != INVALID_SOCKET_VALUE;
    }

    int Socket::Send(const void *data, int size, unsigned int timeout_ms)
    {
        if (timeout_ms > 0 && !Select(false, true, timeout_ms))
            return -1;

        return send(socket_, (const char *)data, size, 0);
    }

    int Socket::Receive(void *buffer, int size, unsigned int timeout_ms)
    {
        if (timeout_ms > 0 && !Select(true, false, timeout_ms))
            return -1;

        return recv(socket_, static_cast<char *>(buffer), size, 0);
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

    bool Socket::Select(bool read, bool write, unsigned int timeout_ms)
    {
        assert(read || write);

#ifdef _WIN32
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(socket_, &fds);

        timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        return select(0, read ? &fds : nullptr, write ? &fds : nullptr, nullptr, &tv) > 0;
#else
        fd_set read_fds, write_fds;
        FD_ZERO(&read_fds);
        FD_ZERO(&write_fds);

        if (read)
            FD_SET(socket_, &read_fds);

        if (write)
            FD_SET(socket_, &write_fds);

        timeval tv;
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int result = 0;
        do
        {
            result = select(socket_ + 1, read ? &read_fds : nullptr, write ? &write_fds : nullptr, nullptr, &tv);
        } while (result == -1 && errno == EINTR);

        return result > 0;
#endif
    }

    bool Socket::SetBlocking(bool blocking)
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