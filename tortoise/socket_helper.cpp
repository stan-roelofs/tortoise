#include "socket_helper.hpp"

namespace tortoise
{
    namespace socket_helper
    {
        // TODO timeout
        bool SendAll(Socket &socket, const void *data, int size, unsigned int timeout)
        {
            int total = 0;
            while (total < size)
            {
                int sent = socket.Send((const char *)data + total, size - total, timeout);
                if (sent == -1)
                    return false;

                total += sent;
            }
            return true;
        }

        bool ReceiveAll(Socket &socket, std::vector<std::uint8_t> &buffer, unsigned int timeout)
        {
            while (true)
            {
                std::uint8_t chunk[1024];
                int received = socket.Receive(chunk, 1024, timeout);
                if (received == -1)
                    return false;
                if (received == 0)
                    return true;

                buffer.insert(buffer.end(), chunk, chunk + received);
            }
        }

        bool ReceiveAll(Socket& socket, void* buffer, int buffer_size, unsigned int timeout)
        {
            int left = buffer_size;
            while (left > 0)
            {
                int received = socket.Receive((std::uint8_t*)buffer + (buffer_size - left), left, timeout);
                if (received == -1)
                    return false;

                left = left - received;
            }

            return true;
        }
    }
}