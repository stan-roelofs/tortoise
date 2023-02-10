#include <tortoise/connection.hpp>

namespace tortoise
{
    std::unique_ptr<Connection> Connection::CreateOutgoing(const std::string &host, const std::string &port)
    {
        std::unique_ptr<Socket> socket = std::make_unique<Socket>();
        if (!socket->Connect(host, port))
            return nullptr;

        return std::unique_ptr<Connection>(new Connection(std::move(socket)));
    }

    std::unique_ptr<Connection> Connection::CreateIncoming(const std::string &port)
    {
        std::unique_ptr<Socket> socket = std::make_unique<Socket>();
        if (!socket->Listen(port))
            return nullptr;

        return std::unique_ptr<Connection>(new Connection(std::move(socket)));
    }

    Connection::Connection(std::unique_ptr<Socket> socket) : socket_(std::move(socket))
    {
    }

    Connection::~Connection()
    {
    }

    int Connection::Send(const void *data, int size)
    {
        return socket_->Send(data, size);
    }

    bool Connection::SendAll(const void *data, int size)
    {
        int total = 0;
        while (total < size)
        {
            int sent = Send((const char *)data + total, size - total);
            if (sent == -1)
                return false;

            total += sent;
        }
        return true;
    }

    int Connection::Receive(void *data, int size)
    {
        return socket_->Receive(data, size);
    }

    bool Connection::ReceiveAll(std::vector<std::uint8_t> &buffer)
    {
        while (true)
        {
            std::uint8_t chunk[1024];
            int received = Receive(chunk, 1024);
            if (received < 0)
                return false;

            if (received == 0)
                return true;

            buffer.insert(buffer.end(), chunk, chunk + received);
        }
    }
} // namespace tortoise