#include "http_tracker_connection.hpp"

#include <optional>
#include <sstream>

#include <tortoise/bencode.hpp>
#include <tortoise/exceptions.hpp>
#include <tortoise/sha1_hash.hpp>
#include <tortoise/url.hpp>

#include "log.hpp"

namespace tortoise
{
    HTTPTrackerConnection::HTTPTrackerConnection(const URL &url) : TrackerConnection(url)
    {
        if (url.GetProtocol() != "http")
            throw UnsupportedProtocolException(url.GetProtocol());
    }

    bool HTTPTrackerConnection::Announce(const AnnounceParameters &parameters, std::function<void(Result, std::shared_ptr<AnnounceResponse> response)> result_callback, unsigned int timeout)
    {
        if (request_)
        {
            LOG("HTTPTrackerConnection", "Announce already in progress");
            return false;
        }

        request_ = std::make_unique<http::AsyncRequest>(url_);
        request_->AddParameter("info_hash", std::string((const char *)parameters.info_hash.GetBytes().data(), parameters.info_hash.GetBytes().size()));
        request_->AddParameter("peer_id", parameters.peer_id.Get());
        request_->AddParameter("port", std::to_string(parameters.port));
        request_->AddParameter("uploaded", std::to_string(parameters.uploaded));
        request_->AddParameter("downloaded", std::to_string(parameters.downloaded));
        request_->AddParameter("left", std::to_string(parameters.left));
        if (parameters.compact.has_value())
            request_->AddParameter("compact", parameters.compact.value() ? "1" : "0");
        if (parameters.no_peer_id.has_value())
            request_->AddParameter("no_peer_id", parameters.no_peer_id.value() ? "1" : "0");
        switch (parameters.event)
        {
        case AnnounceParameters::Event::Started:
            request_->AddParameter("event", "started");
            break;
        case AnnounceParameters::Event::Stopped:
            request_->AddParameter("event", "stopped");
            break;
        case AnnounceParameters::Event::Completed:
            request_->AddParameter("event", "completed");
            break;
        case AnnounceParameters::Event::None:
            break;
        }

        // if (parameters.ip.has_value()) // TODO

        if (parameters.numwant.has_value())
            request_->AddParameter("numwant", std::to_string(parameters.numwant.value()));

        if (parameters.key.has_value())
            request_->AddParameter("key", std::to_string(parameters.key.value()));

        if (parameters.tracker_id.has_value())
            request_->AddParameter("trackerid", parameters.tracker_id.value());

        request_->SetTimeout(timeout);

        return request_->Get([this, result_callback](http::AsyncRequest::Result result, std::shared_ptr<http::Response> response)
                             {
                          if (result != http::AsyncRequest::Result::Success || !response || response->GetStatusCode() != 200)
                          {
                              LOG("HTTPTrackerConnection", "Announce failed");
                              result_callback(Result::Failure, nullptr);
                              return;
                          }

                          auto announce_response = ParseResponse(response->GetBody());
                          if (!announce_response)
                          {
                              LOG("HTTPTrackerConnection", "Announce failed");
                              result_callback(Result::Failure, nullptr);
                              return;
                          }

                          result_callback(Result::Success, announce_response); });
    }

    std::shared_ptr<AnnounceResponse> HTTPTrackerConnection::ParseResponse(const std::string &response_string)
    {
        using namespace bencode;

        std::shared_ptr<AnnounceResponse> response = std::make_shared<AnnounceResponse>();
        std::unique_ptr<Data> data;
        std::istringstream iss(response_string);
        try
        {
            data = Decode(iss);
        }
        catch (BencodeException &e)
        {
            LOG("HTTPTrackerConnection", "Failed to decode response: %s", e.what());
            return nullptr;
        }

        const dictionary_t &dict = Get<dictionary_t>(*data);
        if (dict.find("failure reason") != dict.end())
        {
            response->failure_reason = Get<string_t>(*dict.at("failure reason"));
            return response;
        }

        if (dict.find("warning message") != dict.end())
            response->warning_message = Get<string_t>(*dict.at("warning message"));

        const auto interval_data = dict.find("interval");
        if (interval_data != dict.end())
        {
            integer_t interval = Get<integer_t>(*interval_data->second);
            if (interval < 0)
            {
                LOG("HTTPTrackerConnection", "Interval is negative");
                return nullptr;
            }

            response->interval = interval;
        }
        else
        {
            LOG("HTTPTrackerConnection", "Interval not found");
            return nullptr;
        }

        const auto min_interval_data = dict.find("min interval");
        if (min_interval_data != dict.end())
        {
            integer_t min_interval = Get<integer_t>(*min_interval_data->second);
            if (min_interval < 0)
            {
                LOG("HTTPTrackerConnection", "Min interval is negative");
                return nullptr;
            }

            response->min_interval = min_interval;
        }

        const auto tracker_id = dict.find("tracker id");
        if (tracker_id != dict.end())
            response->tracker_id = Get<string_t>(*tracker_id->second);

        const auto complete_data = dict.find("complete");
        if (complete_data != dict.end())
        {
            integer_t complete = Get<integer_t>(*complete_data->second);
            if (complete < 0)
            {
                LOG("HTTPTrackerConnection", "Complete is negative");
                return nullptr;
            }

            response->complete = complete;
        }
        else
        {
            LOG("HTTPTrackerConnection", "Complete not found");
            return nullptr;
        }

        const auto incomplete_data = dict.find("incomplete");
        if (incomplete_data != dict.end())
        {
            integer_t incomplete = Get<integer_t>(*incomplete_data->second);
            if (incomplete < 0)
            {
                LOG("HTTPTrackerConnection", "Incomplete is negative");
                return nullptr;
            }

            response->incomplete = incomplete;
        }
        else
        {
            LOG("HTTPTrackerConnection", "Incomplete not found");
            return nullptr;
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
                        return nullptr;
                    }

                    AnnounceResponse::PeerInfo info(ip, static_cast<std::uint16_t>(port));
                    info.peer_id = peer_id;
                    response->peers.emplace_back(info);
                }
            }
            else if (CheckType<string_t>(*dict.at("peers")))
            {
                // Compact format - BEP 23
                const string_t &peers = Get<string_t>(*dict.at("peers"));
                if (peers.size() % 6 != 0)
                {
                    LOG("HTTPTrackerConnection", "Invalid peers field");
                    return nullptr;
                }

                for (size_t i = 0; i < peers.size(); i += 6)
                {
                    const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

                    const IPAddress ip = IPAddress(IPAddress::ipv4_address_t{peer[0], peer[1], peer[2], peer[3]});
                    auto port = static_cast<std::uint16_t>((peer[4] << 8) | peer[5]);
                    AnnounceResponse::PeerInfo info(ip.ToString(), port);
                    response->peers.emplace_back(info);
                }
            }
            else
            {
                LOG("HTTPTrackerConnection", "Invalid peers field");
                return nullptr;
            }
        }

        if (dict.find("peers_ipv6") != dict.end())
        {
            if (!CheckType<string_t>(*dict.at("peers")))
            {
                LOG("HTTPTrackerConnection", "Invalid peers_ipv6 field");
                return nullptr;
            }

            const string_t &peers = Get<string_t>(*dict.at("peers_ipv6"));
            if (peers.size() % 18 != 0)
            {
                LOG("HTTPTrackerConnection", "Invalid peers_ipv6 field");
                return nullptr;
            }

            for (size_t i = 0; i < peers.size(); i += 18)
            {
                const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

                const IPAddress ip = IPAddress(IPAddress::ipv6_address_t{
                    peer[0], peer[1], peer[2], peer[3], peer[4], peer[5], peer[6], peer[7], peer[8],
                    peer[9], peer[10], peer[11], peer[12], peer[13], peer[14], peer[15]});

                auto port = static_cast<std::uint16_t>((peer[16] << 8) | peer[17]);
                AnnounceResponse::PeerInfo info(ip.ToString(), port);
                response->peers.emplace_back(info);
            }
        }

        return response;
    }
} // namespace tortoise