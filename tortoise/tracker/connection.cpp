#include "connection.hpp"

namespace tortoise {
namespace tracker {
std::unique_ptr<Connection> Connection::Create(const network::URL &url) {
  if (url.GetProtocol() == "http" || url.GetProtocol() == "https")
    return std::make_unique<HTTPConnection>(url);

  if (url.GetProtocol() == "udp")
    return std::make_unique<UDPConnection>(url);

  return {};
}
} // namespace tracker
} // namespace tortoise