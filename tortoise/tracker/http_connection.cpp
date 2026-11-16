#include <exception>
#include <iomanip>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <tortoise/exception.hpp>

#include "../bencode/bencode.hpp"
#include "../network/url.hpp"
#include "../util/log.hpp"
#include "announce.hpp"
#include "connection.hpp"

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

namespace {
std::string url_encode(const std::string &value) {
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
    unsigned char c = (*i);

    // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << std::uppercase;
    escaped << '%' << std::setw(2) << int((unsigned char)c);
    escaped << std::nouppercase;
  }

  return escaped.str();
}

std::string CreateRequest(const tortoise::network::URL &url, const tortoise::tracker::AnnounceParameters &parameters) {
  std::string request;

  std::string path = url.GetPath();
  // The absolute path cannot be empty; if none is present in the original
  // URI, it MUST be given as "/"
  if (path.empty())
    path = "/";

  std::map<std::string, std::string> params;
  params["info_hash"] = std::string((const char *)parameters.info_hash.data(), parameters.info_hash.size());
  params["peer_id"] = parameters.peer_id.Get();
  params["port"] = std::to_string(parameters.port);
  params["uploaded"] = std::to_string(parameters.uploaded);
  params["downloaded"] = std::to_string(parameters.downloaded);
  params["left"] = std::to_string(parameters.left);
  if (parameters.compact.has_value())
    params["compact"] = parameters.compact.value() ? "1" : "0";
  if (parameters.no_peer_id.has_value())
    params["no_peer_id"] = parameters.no_peer_id.value() ? "1" : "0";
  switch (parameters.event) {
  case tortoise::tracker::AnnounceParameters::Event::Started:
    params["event"] = "started";
    break;
  case tortoise::tracker::AnnounceParameters::Event::Stopped:
    params["event"] = "stopped";
    break;
  case tortoise::tracker::AnnounceParameters::Event::Completed:
    params["event"] = "completed";
    break;
  case tortoise::tracker::AnnounceParameters::Event::None:
    break;
  }

  if (parameters.ip.has_value())
    params["ip"] = parameters.ip->ToString();

  if (parameters.numwant.has_value())
    params["numwant"] = std::to_string(parameters.numwant.value());

  if (parameters.key.has_value())
    params["key"] = std::to_string(parameters.key.value());

  if (parameters.tracker_id.has_value())
    params["trackerid"] = parameters.tracker_id.value();

  std::string query;
  for (const auto &param : params) {
    if (!query.empty())
      query += "&";
    query += url_encode(param.first) + "=" + url_encode(param.second);
  }

  return path + (query.empty() ? "" : "?" + query);
}

struct Response {
public:
  std::string http_version;
  std::uint16_t status_code;
  std::string status_text;
  std::string body;
  std::string headers;
};

std::optional<tortoise::tracker::AnnounceResponse> ParseAnnounceResponse(const std::string &response_string) {
  const auto http_response = response_string;

  using namespace tortoise;
  using namespace bencode;

  tracker::AnnounceResponse response;
  std::unique_ptr<Data> data;
  std::istringstream iss(response_string);
  try {
    data = Decode(iss);
  } catch (BencodeException &e) {
    LOG_ERROR("HTTPTrackerConnection", std::format("Failed to decode response: {}", e.what()));
    return {};
  }

  const dictionary_t &dict = Get<dictionary_t>(*data);
  if (dict.find("failure reason") != dict.end()) {
    response.failure_reason = Get<string_t>(*dict.at("failure reason"));
    return response;
  }

  if (dict.find("warning message") != dict.end())
    response.warning_message = Get<string_t>(*dict.at("warning message"));

  const auto interval_data = dict.find("interval");
  if (interval_data != dict.end()) {
    integer_t interval = Get<integer_t>(*interval_data->second);
    if (interval < 0) {
      LOG_ERROR("HTTPTrackerConnection", "Interval is negative");
      return {};
    }

    response.interval = interval;
  } else {
    LOG_ERROR("HTTPTrackerConnection", "Interval not found");
    return {};
  }

  const auto min_interval_data = dict.find("min interval");
  if (min_interval_data != dict.end()) {
    integer_t min_interval = Get<integer_t>(*min_interval_data->second);
    if (min_interval < 0) {
      LOG_ERROR("HTTPTrackerConnection", "Min interval is negative");
      return {};
    }

    response.min_interval = min_interval;
  }

  const auto tracker_id = dict.find("tracker id");
  if (tracker_id != dict.end())
    response.tracker_id = Get<string_t>(*tracker_id->second);

  const auto complete_data = dict.find("complete");
  if (complete_data != dict.end()) {
    integer_t complete = Get<integer_t>(*complete_data->second);
    if (complete < 0) {
      LOG_ERROR("HTTPTrackerConnection", "Complete is negative");
      return {};
    }

    response.complete = complete;
  } else {
    LOG_ERROR("HTTPTrackerConnection", "Complete not found");
    return {};
  }

  const auto incomplete_data = dict.find("incomplete");
  if (incomplete_data != dict.end()) {
    integer_t incomplete = Get<integer_t>(*incomplete_data->second);
    if (incomplete < 0) {
      LOG_ERROR("HTTPTrackerConnection", "Incomplete is negative");
      return {};
    }

    response.incomplete = incomplete;
  } else {
    LOG_ERROR("HTTPTrackerConnection", "Incomplete not found");
    return {};
  }

  if (dict.find("peers") != dict.end()) {
    if (CheckType<list_t>(*dict.at("peers"))) {
      const list_t &peers = Get<list_t>(*dict.at("peers"));
      for (const auto &peer_dict : peers) {
        const dictionary_t &peer = Get<dictionary_t>(*peer_dict);
        std::string peer_id;
        if (peer.contains("peer id"))
          peer_id = Get<string_t>(*peer.at("peer id"));
        auto ip = Get<string_t>(*peer.at("ip"));
        const integer_t port = Get<integer_t>(*peer.at("port"));
        if (port < 0 || port > 65535) {
          LOG_ERROR("HTTPTrackerConnection", "Invalid port number");
          return {};
        }

        PeerInfo info(ip, static_cast<std::uint16_t>(port));
        try {
          info.peer_id = PeerId::FromString(peer_id);
        } catch (const InvalidArgumentException &) {
          LOG_WARN("HTTPTrackerConnection", std::format("Ignored invalid peer id: {}", peer_id));
        }
        response.peers.emplace_back(info);
      }
    } else if (CheckType<string_t>(*dict.at("peers"))) {
      // Compact format - BEP 23
      const string_t &peers = Get<string_t>(*dict.at("peers"));
      if (peers.size() % 6 != 0) {
        LOG_ERROR("HTTPTrackerConnection", "Invalid peers field");
        return {};
      }

      for (size_t i = 0; i < peers.size(); i += 6) {
        const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

        const network::IPAddress ip = network::IPAddress(network::IPAddress::ipv4_address_t{peer[0], peer[1], peer[2], peer[3]});
        auto port = static_cast<std::uint16_t>((peer[4] << 8) | peer[5]);
        PeerInfo info(ip.ToString(), port);
        response.peers.emplace_back(info);
      }
    } else {
      LOG_ERROR("HTTPTrackerConnection", "Invalid peers field");
      return {};
    }
  }

  if (dict.find("peers_ipv6") != dict.end()) {
    if (!CheckType<string_t>(*dict.at("peers"))) {
      LOG_ERROR("HTTPTrackerConnection", "Invalid peers_ipv6 field");
      return {};
    }

    const string_t &peers = Get<string_t>(*dict.at("peers_ipv6"));
    if (peers.size() % 18 != 0) {
      LOG_ERROR("HTTPTrackerConnection", "Invalid peers_ipv6 field");
      return {};
    }

    for (size_t i = 0; i < peers.size(); i += 18) {
      const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

      const network::IPAddress ip = network::IPAddress(
          network::IPAddress::ipv6_address_t{peer[0], peer[1], peer[2], peer[3], peer[4], peer[5], peer[6], peer[7], peer[8], peer[9],
                                             peer[10], peer[11], peer[12], peer[13], peer[14], peer[15]});

      auto port = static_cast<std::uint16_t>((peer[16] << 8) | peer[17]);
      PeerInfo info(ip.ToString(), port);
      response.peers.emplace_back(info);
    }
  }

  return response;
}
} // namespace

namespace tortoise {
namespace tracker {
HTTPConnection::HTTPConnection(const network::URL &url) : url_(url), client_(std::make_unique<httplib::Client>(url.GetHost())) {
  client_->set_follow_location(true);
  client_->set_connection_timeout(5, 0); // 5 seconds
  client_->set_max_timeout(5000);
  client_->set_keep_alive(true);
}

HTTPConnection::~HTTPConnection() { client_->stop(); }

void HTTPConnection::Cancel() {
  // TODO
}

std::optional<AnnounceResponse> HTTPConnection::Announce(std::shared_ptr<const AnnounceParameters> parameters) {
  LOG_INFO("tracker::HTTPConnection", std::format("Announce ({})", url_.ToString()));

  const auto res = client_->Get(CreateRequest(url_, *parameters));
  if (!res) {
    LOG_ERROR("network error: {}", httplib::to_string(res.error()));
    return {};
  }

  if (res->status >= 400) {
    LOG_ERROR("HTTPTrackerConnection", std::format("http error: {}", res->status));
    return {};
  }
  LOG_INFO("HTTPTrackerConnection", res->body);
  try {
    return ParseAnnounceResponse(res->body);
  } catch (std::exception &e) {
    LOG_ERROR("HTTPTrackerConnection", e.what());
  }
  return {};
}
} // namespace tracker

} // namespace tortoise