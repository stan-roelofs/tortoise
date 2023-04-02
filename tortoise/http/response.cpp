#include "response.hpp"

#include <stdexcept>

#include "exception.hpp"

namespace tortoise
{
    namespace http
    {
        Response::Response(const std::string &response) : response_(response)
        {
            const auto status_line_end = response_.find("\r\n");
            if (status_line_end == std::string::npos)
                throw Exception("Invalid response: " + response_);

            const std::string status_line = response_.substr(0, status_line_end);
            std::size_t space = status_line.find(' ');
            if (space == std::string::npos)
                throw Exception("Invalid status line: " + status_line);

            http_version = status_line.substr(0, space);

            try
            {
                status_code = static_cast<std::uint16_t>(std::stoi(status_line.substr(space + 1, 3)));
            }
            catch (const std::invalid_argument &)
            {
                throw Exception("Invalid status line: " + status_line);
            }

            space = status_line.find(' ', space + 1);
            if (space == std::string::npos)
                throw Exception("Invalid status line: " + status_line);

            status_text = status_line.substr(space + 1);

            std::size_t header_end = response_.find("\r\n\r\n", status_line_end);
            if (header_end == std::string::npos)
                throw Exception("Invalid response: " + response_);

            headers_ = response_.substr(status_line_end + 2, header_end - status_line_end - 2);

            header_end += 4;
            body_ = response_.substr(header_end);
        }

        bool Response::Success() const
        {
            return GetStatusCode() >= 200 && GetStatusCode() < 300;
        }

        const std::string &Response::GetHTTPVersion() const
        {
            return http_version;
        }

        std::uint16_t Response::GetStatusCode() const
        {
            return status_code;
        }

        const std::string &Response::GetStatusText() const
        {
            return status_text;
        }

        const std::string &Response::GetHeaders() const
        {
            return headers_;
        }

        const std::string &Response::GetBody() const
        {
            return body_;
        }
    } // namespace http
}