#include <tortoise/exceptions.hpp>
#include <tortoise/socket.hpp>

#include <cassert>

#ifdef __linux__
#include <fcntl.h>
#endif

namespace tortoise
{
    Socket::Socket(TransportProtocol protocol) : protocol_(protocol), internet_protocol_(InternetProtocol::Unknown), socket_(INVALID_SOCKET_VALUE)
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

        switch (protocol_)
        {
        case TransportProtocol::TCP:
            hints.ai_socktype = SOCK_STREAM;
            break;
        case TransportProtocol::UDP:
            hints.ai_socktype = SOCK_DGRAM;
            break;
        }

        hints.ai_protocol = 0;

        int iResult = getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
        if (iResult != 0)
            return false;

        addrinfo *ptr;
        for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
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
                if (protocol_ == TransportProtocol::TCP && Select(false, true, timeout))
                    break;
            }

            Close();
        }

        freeaddrinfo(result);

        if (socket_ != INVALID_SOCKET_VALUE)
        {
            switch (ptr->ai_family)
            {
            case AF_INET:
                internet_protocol_ = InternetProtocol::IPv4;
                break;
            case AF_INET6:
                internet_protocol_ = InternetProtocol::IPv6;
                break;
            default:
                internet_protocol_ = InternetProtocol::Unknown;
            }
        }

        return socket_ != INVALID_SOCKET_VALUE && internet_protocol_ != InternetProtocol::Unknown;
    }

    bool Socket::Send(const void *data, int size, unsigned int timeout)
    {
        int total = 0;
        while (total < size)
        {
            int sent = SendInternal((const char *)data + total, size - total, timeout);
            if (sent == -1)
                return false;

            total += sent;
        }
        return true;
    }

    bool Socket::ReceiveAll(std::vector<std::uint8_t> &buffer, unsigned int timeout)
    {
        while (true)
        {
            std::uint8_t chunk[1024];
            int received = Receive(chunk, 1024, timeout);
            if (received == -1)
                return false;
            if (received == 0)
                return true;

            buffer.insert(buffer.end(), chunk, chunk + received);
        }
    }

    bool Socket::Receive(void *buffer, int buffer_size, unsigned int timeout)
    {
        int left = buffer_size;
        while (left > 0)
        {
            int received = ReceiveInternal((std::uint8_t *)buffer + (buffer_size - left), left, timeout);
            if (received == -1)
                return false;

            left = left - received;
        }

        return true;
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
            return false;

        if (blocking)
            flags &= ~O_NONBLOCK;
        else
            flags |= O_NONBLOCK;

        return fcntl(socket_, F_SETFL, flags) == 0;
#endif
    }

#define htonll(x) ((1 == htonl(1)) ? (x) : ((uint64_t)htonl((x)&0xFFFFFFFF) << 32) | htonl((x) >> 32))
#define ntohll(x) ((1 == ntohl(1)) ? (x) : ((uint64_t)ntohl((x)&0xFFFFFFFF) << 32) | ntohl((x) >> 32))

    int Socket::SendInternal(const void *data, int size, unsigned int timeout_ms)
    {
        if (timeout_ms > 0 && !Select(false, true, timeout_ms))
            return -1;

        return send(socket_, (const char *)data, size, 0);
    }

    int Socket::ReceiveInternal(void *buffer, int size, unsigned int timeout_ms)
    {
        if (timeout_ms > 0 && !Select(true, false, timeout_ms))
            return -1;

        return recv(socket_, static_cast<char *>(buffer), size, 0);
    }

    std::uint64_t Socket::ToNetworkByteOrder(std::uint64_t value)
    {
        return htonll(value);
    }

    std::uint64_t Socket::FromNetworkByteOrder(std::uint64_t value)
    {
        return ntohll(value);
    }

    std::uint32_t Socket::ToNetworkByteOrder(std::uint32_t value)
    {
        return htonl(value);
    }

    std::uint32_t Socket::FromNetworkByteOrder(std::uint32_t value)
    {
        return ntohl(value);
    }

    std::uint16_t Socket::ToNetworkByteOrder(std::uint16_t value)
    {
        return htons(value);
    }

    std::uint16_t Socket::FromNetworkByteOrder(std::uint16_t value)
    {
        return ntohs(value);
    }

#undef htonll
#undef ntohll
} // namespace tortoise