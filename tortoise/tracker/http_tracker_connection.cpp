#include "http_tracker_connection.hpp"

#include <iomanip>
#include <map>
#include <optional>
#include <sstream>

#include <tortoise/exceptions.hpp>
#include <tortoise/sha1_hash.hpp>

#include "../bencode.hpp"
#include "../log.hpp"
#include "../url.hpp"

namespace
{
    std::string url_encode(const std::string &value)
    {
        std::ostringstream escaped;
        escaped.fill('0');
        escaped << std::hex;

        for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i)
        {
            unsigned char c = (*i);

            // Keep alphanumeric and other accepted characters intact
            if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
            {
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

    std::string CreateRequest(const tortoise::URL &url, const tortoise::AnnounceParameters &parameters)
    {
        std::string request;

        std::string path = url.GetPath();
        // The absolute path cannot be empty; if none is present in the original URI, it MUST be given as "/"
        if (path.empty())
            path = "/";

        std::map<std::string, std::string> params;
        params["info_hash"] = std::string((const char *)parameters.info_hash.GetBytes().data(), parameters.info_hash.GetBytes().size());
        params["peer_id"] = parameters.peer_id.Get();
        params["port"] = std::to_string(parameters.port);
        params["uploaded"] = std::to_string(parameters.uploaded);
        params["downloaded"] = std::to_string(parameters.downloaded);
        params["left"] = std::to_string(parameters.left);
        if (parameters.compact.has_value())
            params["compact"] = parameters.compact.value() ? "1" : "0";
        if (parameters.no_peer_id.has_value())
            params["no_peer_id"] = parameters.no_peer_id.value() ? "1" : "0";
        switch (parameters.event)
        {
        case tortoise::AnnounceParameters::Event::Started:
            params["event"] = "started";
            break;
        case tortoise::AnnounceParameters::Event::Stopped:
            params["event"] = "stopped";
            break;
        case tortoise::AnnounceParameters::Event::Completed:
            params["event"] = "completed";
            break;
        case tortoise::AnnounceParameters::Event::None:
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
        for (const auto &param : params)
        {
            if (!query.empty())
                query += "&";
            query += url_encode(param.first) + "=" + url_encode(param.second);
        }

        request = "GET " + url.GetPath() + (query.empty() ? "" : "?" + query) + " HTTP/1.1\r\n";
        request += "Host: " + url.GetHost() + "\r\n";
        request += "Connection: close\r\n";
        request += "\r\n";
        return request;
    }

    struct Response
    {
    public:
        std::string http_version;
        std::uint16_t status_code;
        std::string status_text;
        std::string body;
        std::string headers;
    };

    std::optional<Response> ParseResponse(const std::string &response)
    {
        Response result;
        const auto status_line_end = response.find("\r\n");
        if (status_line_end == std::string::npos)
            return {};

        const std::string status_line = response.substr(0, status_line_end);
        std::size_t space = status_line.find(' ');
        if (space == std::string::npos)
            return {};

        result.http_version = status_line.substr(0, space);

        try
        {
            result.status_code = static_cast<std::uint16_t>(std::stoi(status_line.substr(space + 1, 3)));
        }
        catch (const std::invalid_argument &)
        {
            return {};
        }

        space = status_line.find(' ', space + 1);
        if (space == std::string::npos)
            return {};

        result.status_text = status_line.substr(space + 1);

        std::size_t header_end = response.find("\r\n\r\n", status_line_end);
        if (header_end == std::string::npos)
            return {};

        result.headers = response.substr(status_line_end + 2, header_end - status_line_end - 2);

        header_end += 4;
        result.body = response.substr(header_end);
        return result;
    }

    std::optional<tortoise::AnnounceResponse> ParseAnnounceResponse(const std::string &response_string)
    {
        const auto http_response = ParseResponse(response_string);
        if (!http_response)
            return {};

        using namespace tortoise;
        using namespace bencode;

        AnnounceResponse response;
        std::unique_ptr<Data> data;
        std::istringstream iss(http_response->body);
        try
        {
            data = Decode(iss);
        }
        catch (BencodeException &e)
        {
            LOG("HTTPTrackerConnection", "Failed to decode response: %s", e.what());
            return {};
        }

        const dictionary_t &dict = Get<dictionary_t>(*data);
        if (dict.find("failure reason") != dict.end())
        {
            response.failure_reason = Get<string_t>(*dict.at("failure reason"));
            return response;
        }

        if (dict.find("warning message") != dict.end())
            response.warning_message = Get<string_t>(*dict.at("warning message"));

        const auto interval_data = dict.find("interval");
        if (interval_data != dict.end())
        {
            integer_t interval = Get<integer_t>(*interval_data->second);
            if (interval < 0)
            {
                LOG("HTTPTrackerConnection", "Interval is negative");
                return {};
            }

            response.interval = interval;
        }
        else
        {
            LOG("HTTPTrackerConnection", "Interval not found");
            return {};
        }

        const auto min_interval_data = dict.find("min interval");
        if (min_interval_data != dict.end())
        {
            integer_t min_interval = Get<integer_t>(*min_interval_data->second);
            if (min_interval < 0)
            {
                LOG("HTTPTrackerConnection", "Min interval is negative");
                return {};
            }

            response.min_interval = min_interval;
        }

        const auto tracker_id = dict.find("tracker id");
        if (tracker_id != dict.end())
            response.tracker_id = Get<string_t>(*tracker_id->second);

        const auto complete_data = dict.find("complete");
        if (complete_data != dict.end())
        {
            integer_t complete = Get<integer_t>(*complete_data->second);
            if (complete < 0)
            {
                LOG("HTTPTrackerConnection", "Complete is negative");
                return {};
            }

            response.complete = complete;
        }
        else
        {
            LOG("HTTPTrackerConnection", "Complete not found");
            return {};
        }

        const auto incomplete_data = dict.find("incomplete");
        if (incomplete_data != dict.end())
        {
            integer_t incomplete = Get<integer_t>(*incomplete_data->second);
            if (incomplete < 0)
            {
                LOG("HTTPTrackerConnection", "Incomplete is negative");
                return {};
            }

            response.incomplete = incomplete;
        }
        else
        {
            LOG("HTTPTrackerConnection", "Incomplete not found");
            return {};
        }

        if (dict.find("peers") != dict.end())
        {
            if (CheckType<list_t>(*dict.at("peers")))
            {
                const list_t &peers = Get<list_t>(*dict.at("peers"));
                for (const auto &peer_dict : peers)
                {
                    const dictionary_t &peer = Get<dictionary_t>(*peer_dict);
                    auto peer_id = Get<string_t>(*peer.at("peer id"));
                    auto ip = Get<string_t>(*dict.at("ip"));
                    const integer_t port = Get<integer_t>(*dict.at("port"));
                    if (port < 0 || port > 65535)
                    {
                        LOG("HTTPTrackerConnection", "Invalid port number");
                        return {};
                    }

                    AnnounceResponse::PeerInfo info(ip, static_cast<std::uint16_t>(port));
                    info.peer_id = peer_id;
                    response.peers.emplace_back(info);
                }
            }
            else if (CheckType<string_t>(*dict.at("peers")))
            {
                // Compact format - BEP 23
                const string_t &peers = Get<string_t>(*dict.at("peers"));
                if (peers.size() % 6 != 0)
                {
                    LOG("HTTPTrackerConnection", "Invalid peers field");
                    return {};
                }

                for (size_t i = 0; i < peers.size(); i += 6)
                {
                    const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

                    const IPAddress ip = IPAddress(IPAddress::ipv4_address_t{peer[0], peer[1], peer[2], peer[3]});
                    auto port = static_cast<std::uint16_t>((peer[4] << 8) | peer[5]);
                    AnnounceResponse::PeerInfo info(ip.ToString(), port);
                    response.peers.emplace_back(info);
                }
            }
            else
            {
                LOG("HTTPTrackerConnection", "Invalid peers field");
                return {};
            }
        }

        if (dict.find("peers_ipv6") != dict.end())
        {
            if (!CheckType<string_t>(*dict.at("peers")))
            {
                LOG("HTTPTrackerConnection", "Invalid peers_ipv6 field");
                return {};
            }

            const string_t &peers = Get<string_t>(*dict.at("peers_ipv6"));
            if (peers.size() % 18 != 0)
            {
                LOG("HTTPTrackerConnection", "Invalid peers_ipv6 field");
                return {};
            }

            for (size_t i = 0; i < peers.size(); i += 18)
            {
                const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

                const IPAddress ip = IPAddress(IPAddress::ipv6_address_t{
                    peer[0], peer[1], peer[2], peer[3], peer[4], peer[5], peer[6], peer[7], peer[8],
                    peer[9], peer[10], peer[11], peer[12], peer[13], peer[14], peer[15]});

                auto port = static_cast<std::uint16_t>((peer[16] << 8) | peer[17]);
                AnnounceResponse::PeerInfo info(ip.ToString(), port);
                response.peers.emplace_back(info);
            }
        }

        return response;
    }
}

namespace tortoise
{

    HTTPTrackerConnection::HTTPTrackerConnection(const URL &url)
        : TrackerConnection(url),
          socket_(Socket::TransportProtocol::TCP),
          state_(State::Idle),
          current_buffer_position_(0)
    {
        if (url.GetProtocol() != "http")
            throw UnsupportedProtocolException(url.GetProtocol());
    }

    HTTPTrackerConnection::~HTTPTrackerConnection()
    {
        // TODO
    }

    bool HTTPTrackerConnection::Announce(const AnnounceParameters &parameters)
    {
        if (state_ != State::Idle)
        {
            LOG("HTTPTrackerConnection", "Announce called while connection is not idle");
            return false;
        }

        if (!socket_.Connect(url_.GetHost(), url_.GetPort().empty() ? "80" : url_.GetPort()))
        {
            LOG("HTTPTrackerConnection", "Failed to connect to tracker");
            return false;
        }

        std::string request = CreateRequest(url_, parameters);
        buffer_.resize(request.size());
        std::copy(request.begin(), request.end(), buffer_.begin());

        state_ = State::Connect;
        return true;
    }

    bool HTTPTrackerConnection::Process()
    {
        switch (state_)
        {
        case State::Idle:
            return true;
        case State::Connect:
        {
            if (!socket_.Connected())
                return false;

            state_ = State::SendRequest;
        }
            [[fallthrough]];
        case State::SendRequest:
        {
            int length = (int)(buffer_.size() - current_buffer_position_);
            const auto result = socket_.Send(buffer_.data() + current_buffer_position_, length);
            if (result == Socket::Result::Error)
            {
                state_ = State::Idle;
                result_ = {false, {}};
                return true;
            }

            if (result == Socket::Result::WouldBlock)
                return false;

            current_buffer_position_ += length;
            if (current_buffer_position_ < buffer_.size())
                return false;

            buffer_.clear();
            state_ = State::ReceiveResponse;
            return false;
        }
        case State::ReceiveResponse:
        {
            std::uint8_t temp[1024];
            int length = sizeof(temp);
            const auto result = socket_.Receive(temp, length);
            if (result == Socket::Result::Error)
            {
                state_ = State::Idle;
                result_ = {false, {}};
                return true;
            }

            if (result == Socket::Result::WouldBlock)
                return false;

            std::copy(temp, temp + length, std::back_inserter(buffer_));

            if (length == 0)
            {
                state_ = State::Idle;
                const auto response = ParseAnnounceResponse(std::string(buffer_.begin(), buffer_.end()));
                result_ = {response.has_value(), response};
            }
            return false;
        }
        }

        return false;
    }

    TrackerConnection::Result HTTPTrackerConnection::GetLastResult() const
    {
        return result_;
    }

    void HTTPTrackerConnection::Cancel()
    {
        state_ = State::Idle;
    }

} // namespace tortoise