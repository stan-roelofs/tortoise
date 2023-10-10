#include "socket.hpp"

#include <cassert>

#ifdef __linux__
#include <fcntl.h>
#endif

#include "log.hpp"

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
        Close();

#ifdef _WIN32
        WSACleanup();
#endif
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
        if (host.empty() || port.empty())
        {
            LOG("Socket", "host or port is empty");
            return false;
        }

        LOG("Socket", "Connecting to %s:%s", host.c_str(), port.c_str());

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
        {
            LOG("Socket", "getaddrinfo failed: %d", iResult);
            return false;
        }

        addrinfo *ptr;
        for (ptr = result; ptr != nullptr; ptr = ptr->ai_next)
        {
            socket_ = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
            if (socket_ == INVALID_SOCKET_VALUE)
            {
                LOG("Socket", "Failed to create socket");
                continue;
            }

            if (!SetBlocking(false))
            {
                LOG("Socket", "Failed to set socket to non-blocking mode");
                return false;
            }

            if (connect(socket_, ptr->ai_addr, (int)ptr->ai_addrlen) == 0)
                break;

#ifdef _WIN32
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK)
                break;
#else
            if (errno == EINPROGRESS)
                break;
#endif

            Close();
        }

        freeaddrinfo(result);

        if (socket_ != INVALID_SOCKET_VALUE && ptr != nullptr && ptr->ai_addr != nullptr)
        {
            switch (ptr->ai_addr->sa_family)
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

    Socket::Result Socket::Send(const void *data, int &length)
    {
        const int bytes_sent = send(socket_, static_cast<const char *>(data), length, 0);
        if (bytes_sent >= 0)
        {
            assert(bytes_sent == length);
            length = bytes_sent;
            return Result::Ok;
        }

        length = 0;

#ifdef _WIN32
        const int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
            return Result::WouldBlock;

        LOG("Socket", "Send failed with error code: %i", error);
        return Result::Error;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return Result::WouldBlock;

        LOG("Socket", "Send failed with error code: %i", errno);
        return Result::Error;
#endif
    }

    Socket::Result Socket::Receive(void *buffer, int &length)
    {
        const int bytes_received = recv(socket_, static_cast<char *>(buffer), length, 0);
        if (bytes_received >= 0)
        {
            length = bytes_received;
            return Result::Ok;
        }

        length = 0;

#ifdef _WIN32
        const int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK)
            return Result::WouldBlock;

        LOG("Socket", "Receive failed with error code: %i", error);
        return Result::Error;
#else
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return Result::WouldBlock;

        LOG("Socket", "Receive failed with error code: %i", errno);
        return Result::Error;
#endif
    }

    Socket::Status Socket::GetConnectionStatus() const
    {
        if (socket_ == INVALID_SOCKET_VALUE)
            return Status::Error;

        int error = 0;
        socklen_t len = sizeof(error);
        if (getsockopt(socket_, SOL_SOCKET, SO_ERROR, (char *)&error, &len) < 0)
        {
            const int code =
#ifdef _WIN32
                WSAGetLastError();
#else
                code;
#endif
            (void)code;
            LOG("Socket", "getsockopt failed: %i", code);
            return Status::Error;
        }

        if (error != 0)
            LOG("Socket", "Socket error: %i", error);

        if (!Select(false, true, 1))
            return Status::Pending;

        return (error == 0) ? Status::Connected : Status::Error;
    }

    Socket::TransportProtocol Socket::GetTransportProtocol() const
    {
        return protocol_;
    }

    Socket::InternetProtocol Socket::GetInternetProtocol() const
    {
        return internet_protocol_;
    }

    bool Socket::Select(bool read, bool write, unsigned int timeout_us) const
    {
        assert(read || write);

        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(socket_, &fds);

        timeval tv;
        tv.tv_sec = timeout_us / 1000000;
        tv.tv_usec = (timeout_us % 1000000);

#ifdef _WIN32
        return select(0, read ? &fds : nullptr, write ? &fds : nullptr, &fds, &tv) > 0;
#else
        int result = 0;
        do
        {
            result = select(socket_ + 1, read ? &fds : nullptr, write ? &fds : nullptr, &fds, &tv);
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