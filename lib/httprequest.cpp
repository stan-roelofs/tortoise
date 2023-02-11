#include "httprequest.hpp"

#include <cctype>
#include <iomanip>
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
    HTTPRequest::HTTPRequest(const std::string &url, const std::map<std::string, std::string> &params)
    {
        std::size_t pos = url.find("://");
        if (pos == std::string::npos)
			throw HTTPRequestException("Invalid URL, missing protocol");

        std::string protocol = url.substr(0, pos);
		if (protocol != "http")
			throw HTTPRequestException("Invalid URL, unsupported protocol: " + protocol);

		host_ = url.substr(pos + 3);

        const size_t path_start = host_.find("/");
        std::string path = "/";
        if (path_start != std::string::npos)
        {
            path = host_.substr(path_start);
            host_ = host_.substr(0, path_start);
        }

        const size_t port_start = host_.find(":");
        port_ = "80";
        if (port_start != std::string::npos)
        {
            port_ = host_.substr(port_start + 1);
            host_ = host_.substr(0, port_start);
        }

        std::stringstream args;
        for (const auto &param : params)
        {
            if (args.tellp() != 0)
                args << "&";
            args << param.first << "=" << url_encode(param.second);
        }

        std::stringstream request;
        request << "GET " << path << "?" << args.str() << " HTTP/1.1\r\n";
        request << "Host: " << host_ << "\r\n";
        request << "\r\n";
		request_ = request.str();
    }

    HTTPRequest::~HTTPRequest()
    {
    }

    std::string HTTPRequest::Get() const
    {       
        if (request_.size() > (std::numeric_limits<int>::max)())
            throw HTTPRequestException("Request too large");

        Socket socket;
		if (!socket.Connect(host_, port_))
			throw HTTPRequestException("Failed to connect to host");

        if (!socket_helper::SendAll(socket, request_.c_str(), static_cast<int>(request_.size())))
			throw HTTPRequestException("Failed to send request");

        std::vector<std::uint8_t> buffer;
		if (!socket_helper::ReceiveAll(socket, buffer))
			throw HTTPRequestException("Failed to receive response");
        
        return GetBodyFromResponse(std::string(buffer.begin(), buffer.end()));
    }

    std::string HTTPRequest::GetBodyFromResponse(const std::string& response)
	{
		const std::string status_line = response.substr(0, response.find("\r\n"));
		std::string status_code = status_line.substr(status_line.find(" ") + 1);
		if (status_code.at(0) != '2')
			throw HTTPRequestException("Request failed with status code: " + status_code);
                
		return response.substr(response.find("\r\n\r\n") + 4);
	}
} // namespace tortoise