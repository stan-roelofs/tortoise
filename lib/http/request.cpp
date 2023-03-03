#include <tortoise/http/request.hpp>

#include <iomanip>
#include <sstream>

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
    namespace http
    {
        AsyncRequest::AsyncRequest(const URL &url, const std::map<std::string, std::string> &params) : state_(State::Connecting), bytes_sent_(0)
        {
            CreateRequest(url, params);
        }

        void AsyncRequest::CreateRequest(const URL &url, const std::map<std::string, std::string> &params)
        {
            if (url.GetProtocol() != "http")
                throw Exception("Unsupported protocol: " + url.GetProtocol());

            std::string path = url.GetPath();
            // The absolute path cannot be empty; if none is present in the original URI, it MUST be given as "/"
            if (path.empty())
                path = "/";

            std::string query;
            for (const auto &param : params)
            {
                if (!query.empty())
                    query += "&";
                query += url_encode(param.first) + "=" + url_encode(param.second);
            }

            request_ = "GET " + url.GetPath() + (query.empty() ? "" : "?" + query) + " HTTP/1.1\r\n";
            request_ += "Host: " + url.GetHost() + "\r\n";
            request_ += "Connection: close\r\n";
            request_ += "\r\n";

            socket_.SetBlocking(false);
            if (!socket_.Connect(url.GetHost(), url.GetPort()))
                throw Exception("Failed to connect to " + url.GetHost() + ":" + url.GetPort());
        }

        bool AsyncRequest::Process()
        {
            if (state_ == State::Connecting && socket_.Connected())
                state_ = State::Sending;

            if (state_ == State::Sending)
            {
                int sent = socket_.Send(request_.c_str(), static_cast<int>(request_.size()));
                if (sent > 0)
                    bytes_sent_ += sent;

                if (bytes_sent_ == request_.size())
                    state_ = State::Receiving;

                return false;
            }

            if (state_ == State::Receiving)
            {
                char buffer[0xFFFF];
                int received = socket_.Receive(buffer, 0xFFFF);
                if (received == 0)
                {
                    state_ = State::Done;
                    try
                    {
                        response_ = std::make_shared<Response>(response_buffer_);
                    }
                    catch (const Exception &)
                    {
                    }
                    return true;
                }
                else if (received > 0)
                {
                    response_buffer_ += std::string(buffer, received);
                }
            }

            return false;
        }

        bool AsyncRequest::Done() const
        {
            return state_ == State::Done;
        }

		std::shared_ptr<Response> AsyncRequest::GetResponse() const
		{
			return response_;
		}
    } // namespace http
} // namespace tortoise