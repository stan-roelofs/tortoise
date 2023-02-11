#include <tortoise/tracker.hpp>

#include <sstream>

#include <tortoise/exceptions.hpp>
#include <tortoise/url.hpp>

#include "httprequest.hpp"
#include "log.hpp"

namespace
{
    constexpr const char *DEFAULT_HTTP_PORT = "80";
}

namespace tortoise
{
    Tracker::Tracker(const Metainfo &info) : info_(info), interval_(0), min_interval_(0), complete_(0), incomplete_(0)
    {
    }

    bool Tracker::Announce()
    {
        const URL url(info_.GetAnnounce());
        if (!socket_.Connect(url.GetHost(), url.GetPort().empty() ? DEFAULT_HTTP_PORT : url.GetPort()))
        {
            LOG("Tracker", "Failed to connect to tracker");
            return false;
        }

        // TODO Check the minimum interval?

        std::map<std::string, std::string> params;
        auto hash = info_.GetInfoHash();
        params["info_hash"] = std::string((const char *)hash.data(), hash.size());
        // params["peer_id"] = std::string("12345678901234567890", 20); // TODO set this properly
        // params["port"] = std::to_string(6881);
        // params["uploaded"] = std::to_string(0);
        // params["downloaded"] = std::to_string(0);
        // params["left"] = "500";      // TODO
        params["compact"] = "1";
        // params["no_peer_id"] = "0";  // TODO
        params["event"] = "started";

        try
        {
            HTTPRequest request(info_.GetAnnounce(), params);
            return HandleResponse(request.Get(socket_));
        }
        catch (HTTPRequestException &e)
        {
            LOG("Tracker", "Sending announce failed: %s", e.what());
            return false;
        }
    }

    uint64_t Tracker::GetInterval() const
    {
        return interval_;
    }

    uint64_t Tracker::GetMinimumInterval() const
    {
        return min_interval_;
    }

    uint64_t Tracker::GetComplete() const
    {
        return complete_;
    }

    uint64_t Tracker::GetIncomplete() const
    {
        return incomplete_;
    }

    const std::vector<Tracker::PeerInfo> &Tracker::GetPeers() const
    {
        return peers_;
    }

    bool Tracker::HandleResponse(const std::string &response)
    {
        using namespace bencode;

        std::unique_ptr<Data> data;
        std::istringstream iss(response);
        try
        {
            data = Decode(iss);
        }
        catch (BencodeException &e)
        {
            LOG("Tracker", "Failed to decode response: %s", e.what());
            return false;
        }

        const dictionary_t &dict = Get<dictionary_t>(*data);
        if (dict.find("failure reason") != dict.end())
        {
            LOG("Tracker", "Request failed: %s", Get<string_t>(*dict.at("failure reason")).c_str());
            return false;
        }

        if (dict.find("warning message") != dict.end())
        {
            LOG("Tracker", "Warning: %s", Get<string_t>(*dict.at("warning message")).c_str());
        }

        const auto interval_data = dict.find("interval");
        if (interval_data != dict.end())
        {
            integer_t interval = Get<integer_t>(*interval_data->second);
            if (interval < 0)
            {
                LOG("Tracker", "Interval is negative");
                return false;
            }

            interval_ = interval;
        }
        else
        {
            LOG("Tracker", "No interval in response");
            return false;
        }

        const auto min_interval_data = dict.find("min interval");
        if (min_interval_data != dict.end())
        {
            integer_t min_interval = Get<integer_t>(*min_interval_data->second);
            if (min_interval < 0)
            {
                LOG("Tracker", "Min interval is negative");
                return false;
            }

            min_interval_ = min_interval;
        }
        else
            min_interval_ = 0;

        const auto tracker_id = dict.find("tracker id");
        if (tracker_id != dict.end())
            tracker_id_ = Get<string_t>(*tracker_id->second);

        const auto complete_data = dict.find("complete");
        if (complete_data != dict.end())
        {
            integer_t complete = Get<integer_t>(*complete_data->second);
            if (complete < 0)
            {
                LOG("Tracker", "Complete is negative");
                return false;
            }

            complete_ = complete;
        }
        else
            complete_ = 0;

        const auto incomplete_data = dict.find("incomplete");
        if (incomplete_data != dict.end())
        {
            integer_t incomplete = Get<integer_t>(*incomplete_data->second);
            if (incomplete < 0)
            {
                LOG("Tracker", "Incomplete is negative");
                return false;
            }

            incomplete_ = incomplete;
        }
        else
            incomplete_ = 0;

        if (dict.find("peers") != dict.end())
        {
            if (CheckType<list_t>(*dict.at("peers")))
            {
                const list_t &peers = Get<list_t>(*dict.at("peers"));
                for (const auto &peer_dict : peers)
                {
                    const dictionary_t &peer = Get<dictionary_t>(*peer_dict);
                    PeerInfo info;
                    info.peer_id = Get<string_t>(*peer.at("peer id"));
                    info.ip = Get<string_t>(*dict.at("ip"));
                    const integer_t port = Get<integer_t>(*dict.at("port"));
                    if (port < 0 || port > 65535)
                    {
                        LOG("Tracker", "Invalid port number");
                        return false;
                    }
                    peers_.emplace_back(info);
                }
            }
            else if (CheckType<string_t>(*dict.at("peers")))
            {
                const string_t &peers = Get<string_t>(*dict.at("peers"));
                if (peers.size() % 6 != 0)
                {
                    LOG("Tracker", "Invalid peers field");
                    return false;
                }

                for (size_t i = 0; i < peers.size(); i += 6)
                {
                    const std::uint8_t *peer = (const uint8_t *)(peers.data() + i);

                    PeerInfo info;
                    for (size_t j = 0; j < 4; ++j)
                    {
                        info.ip += std::to_string(*(peer + j));
                        if (j < 3)
                            info.ip += ".";
                    }
                    info.port = static_cast<std::uint16_t>((peer[4] << 8) | peer[5]);
                    peers_.emplace_back(info);
                }
            }
            else
            {
                LOG("Tracker", "Invalid peers field");
                return false;
            }
        }

        return true;
    }

} // namespace tortoise