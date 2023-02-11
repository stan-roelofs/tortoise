#include "httprequest.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>
#include <string>

#include <tortoise/connection.hpp>
#include <tortoise/exceptions.hpp>

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
    std::unique_ptr<HTTPRequest> HTTPRequest::Create(const std::string &url, const std::map<std::string, std::string> &params)
    {
        std::size_t pos = url.find("://");
        if (pos == std::string::npos)
            return nullptr;

        std::string protocol = url.substr(0, pos);
        std::string host;
        if (protocol == "https" || protocol == "http") // TODO support https properly
            host = url.substr(pos + 3);
        else
            return {};

        const size_t path_start = host.find("/");
        std::string path = "/";
        if (path_start != std::string::npos)
        {
            path = host.substr(path_start);
            host = host.substr(0, path_start);
        }

        const size_t port_start = host.find(":");
        std::string port = "80";
        if (port_start != std::string::npos)
        {
            port = host.substr(port_start + 1);
            host = host.substr(0, port_start);
        }

        std::stringstream args;
        for (const auto &param : params)
        {
            if (args.tellp() == 0)
                args << "&";
            args << param.first << "=" << url_encode(param.second);
        }

        std::stringstream request;
        request << "GET " << path << "?" << args.str() << " HTTP/1.1\r\n";
        request << "Host: " << host << "\r\n";
        request << "\r\n";
        
        std::unique_ptr<Connection> connection = Connection::CreateOutgoing(host, port);
        if (!connection)
            return nullptr;

        return std::unique_ptr<HTTPRequest>(new HTTPRequest(std::move(connection), request.str()));
    }

    HTTPRequest::HTTPRequest(std::unique_ptr<Connection> connection, const std::string &request) : connection_(std::move(connection)), request_(request)
    {
    }

    HTTPRequest::~HTTPRequest()
    {
    }

    bool HTTPRequest::Get(std::string &response)
    {
        if (request_.empty())
            return false;

        if (request_.size() > (std::numeric_limits<int>::max)())
            throw HTTPRequestException("Request too large");

        if (!connection_->SendAll(request_.c_str(), static_cast<int>(request_.size())))
            return false;

        std::vector<std::uint8_t> buffer;
        if (!connection_->ReceiveAll(buffer))
            return false;

        response = std::string(buffer.begin(), buffer.end());

        return true;
    }
} // namespace tortoise