#include "tracker_connection.hpp"

#include "url.hpp"

namespace tortoise
{
    TrackerConnection::TrackerConnection(const URL &url) : url_(url) {}

    TrackerConnection::~TrackerConnection() = default;
} // namespace tortoise