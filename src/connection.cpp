#include <tortoise/connection.hpp>

namespace tortoise
{
    class Connection
    {
        std::unique_ptr<Connection> Connection::CreateOutgoing(const std::string &host, const std::string &port)
        {
            std::unique_ptr<Socket> socket = std::make_unique<Socket>();
            if (!socket->Connect(host, port))
                return nullptr;

            return std::make_unique<Connection>(std::move(socket));
        }

        std::unique_ptr<Connection> Connection::CreateIncoming(const std::string &port)
        {
            std::unique_ptr<Socket> socket = std::make_unique<Socket>();
            if (!socket->Listen(port))
                return nullptr;

            return std::make_unique<Connection>(std::move(socket));
        }

        Connection::Connection(std::unique_ptr<Socket> socket) : socket_(std::move(socket))
        {
        }

        Connection::~Connection()
        {
        }
    };
} // namespace tortoise