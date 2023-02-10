#include <tortoise/tracker.hpp>

#include "httprequest.hpp"

namespace tortoise
{
    Tracker::Tracker(const Metainfo &info) : info_(info)
    {
    }

    bool Tracker::SendAnnounce()
    {
        std::map<std::string, std::string> params;
        auto hash = info_.GetInfoHash();
        params["info_hash"] = std::string((const char *)hash.data(), hash.size());
        params["peer_id"] = std::string("12345678901234567890", 20); // TODO set this properly
        params["port"] = std::to_string(6881);
        params["uploaded"] = std::to_string(0);
        params["downloaded"] = std::to_string(0);
        params["left"] = "500";      // TODO
        params["compact"] = "0";     // TODO
        params["no_peer_id"] = "0";  // TODO
        params["event"] = "started"; // TODO

        auto request = HTTPRequest::Create(info_.GetAnnounce(), params);
        if (!request)
            return false;

        std::string response;
        if (!request->Get(response))
            return false;

        return true;
    }
} // namespace tortoise