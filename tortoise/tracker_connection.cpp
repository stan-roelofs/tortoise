#include "tracker_connection.hpp"

#include <sstream>

#include <tortoise/bencode.hpp>
#include <tortoise/exceptions.hpp>
#include <tortoise/sha1_hash.hpp>

#include "http_tracker_connection.hpp"
#include "log.hpp"
#include "udp_tracker_connection.hpp"
#include "url.hpp"

namespace tortoise
{
    TrackerConnection::TrackerConnection(const URL &url) : url_(url) {}

    TrackerConnection::~TrackerConnection() = default;
} // namespace tortoise