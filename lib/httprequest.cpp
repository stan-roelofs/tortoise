#include "httprequest.hpp"

#include <cctype>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

#include <tortoise/exceptions.hpp>

#include "socket_helper.hpp"

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
}

namespace tortoise
{
    HTTPRequest::HTTPRequest(const URL &url, const std::map<std::string, std::string> &params)
    {
        if (url.GetProtocol() != "http")
            throw HTTPRequestException("Invalid URL, unsupported protocol: " + url.GetProtocol());

        std::string path = url.GetPath();
        // The absolute path cannot be empty; if none is present in the original URI, it MUST be given as "/"
        if (path.empty())
            path = "/";

        std::stringstream args;
        for (const auto &param : params)
        {
            if (args.tellp() != 0)
                args << "&";
            args << param.first << "=" << url_encode(param.second);
        }

        std::stringstream request;
        request << "GET " << path << "?" << args.str() << " HTTP/1.1\r\n";
        request << "Host: " << url.GetHost() << "\r\n";
        request << "\r\n";
        request_ = request.str();
    }

    HTTPRequest::~HTTPRequest()
    {
    }

    std::string HTTPRequest::Get(Socket &socket) const
    {
        if (request_.size() > (std::numeric_limits<int>::max)())
            throw HTTPRequestException("Request too large");

        if (!socket_helper::SendAll(socket, request_.c_str(), static_cast<int>(request_.size())))
            throw HTTPRequestException("Failed to send request");

        std::vector<std::uint8_t> buffer;
        if (!socket_helper::ReceiveAll(socket, buffer))
            throw HTTPRequestException("Failed to receive response");

        return GetBodyFromResponse(std::string(buffer.begin(), buffer.end()));
    }

    std::string HTTPRequest::GetBodyFromResponse(const std::string &response)
    {
        const std::string status_line = response.substr(0, response.find("\r\n"));
        std::string status_code = status_line.substr(status_line.find(" ") + 1);
        if (status_code.at(0) != '2')
            throw HTTPRequestException("Request failed with status code: " + status_code);

        return response.substr(response.find("\r\n\r\n") + 4);
    }
} // namespace tortoise