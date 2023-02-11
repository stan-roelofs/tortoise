#include "socket_helper.hpp"

namespace tortoise
{
    namespace socket_helper
    {
        bool SendAll(Socket &socket, const void *data, int size)
        {
            int total = 0;
            while (total < size)
            {
                int sent = socket.Send((const char *)data + total, size - total);
                if (sent == -1)
                    return false;

                total += sent;
            }
            return true;
        }

        bool ReceiveAll(Socket &socket, std::vector<std::uint8_t> &buffer)
        {
            while (true)
            {
                std::uint8_t chunk[1024];
                int received = socket.Receive(chunk, 1024);
                if (received == -1)
                    return false;
                if (received == 0)
                    return true;

                buffer.insert(buffer.end(), chunk, chunk + received);
            }
        }
    }
}