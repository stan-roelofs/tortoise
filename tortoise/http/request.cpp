#include <tortoise/http/request.hpp>

#include <iomanip>
#include <sstream>

#include "../log.hpp"
#include "../socket_helper.hpp"

namespace
{
    constexpr unsigned int DEFAULT_TIMEOUT_MS = 30000;

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
        // TODO mutex
        AsyncRequest::AsyncRequest(const URL &url) : url_(url), timeout_(DEFAULT_TIMEOUT_MS)
        {
        }

        AsyncRequest::~AsyncRequest()
        {
            // TODO cancel
        }

        std::string AsyncRequest::CreateRequest(const URL &url, const std::map<std::string, std::string> &params)
        {
            std::string request;

            if (url.GetProtocol() != "http")
                throw UnsupportedProtocolException(url.GetProtocol());

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

            request = "GET " + url.GetPath() + (query.empty() ? "" : "?" + query) + " HTTP/1.1\r\n";
            request += "Host: " + url.GetHost() + "\r\n";
            request += "Connection: close\r\n";
            request += "\r\n";
            return request;
        }

        void AsyncRequest::AddParameter(const std::string &key, const std::string &value)
        {
            params_[key] = value;
        }

        void AsyncRequest::SetTimeout(unsigned int timeout)
        {
            timeout_ = timeout;
        }

        bool AsyncRequest::Get(std::function<void(Result result, std::shared_ptr<Response> response)> callback)
        {
            if (thread_.joinable())
                return false;

            request_ = CreateRequest(url_, params_);

            callback_ = callback;
            thread_ = std::thread(&AsyncRequest::ThreadFunc, this);
            return true;
        }

        void AsyncRequest::ThreadFunc(AsyncRequest *request)
        {
            request->SendRequest();
        }

        void AsyncRequest::SendRequest()
        {
            Socket socket(Socket::TransportProtocol::TCP);
            if (!socket.Connect(url_.GetHost(), url_.GetPort(), timeout_))
            {
                callback_(Result::Failure, nullptr);
                return;
            }

            LOG("AsyncRequest", "Connected to %s:%s", url_.GetHost().c_str(), url_.GetPort().c_str());

            if (!socket_helper::SendAll(socket, request_.c_str(), timeout_))
            {
                callback_(Result::Failure, nullptr);
                return;
            }

            LOG("AsyncRequest", "Sent request:\n%s", request_.c_str());

            std::vector<std::uint8_t> response;
            if (!socket_helper::ReceiveAll(socket, response, timeout_))
            {
                callback_(Result::Failure, nullptr);
                return;
            }

            LOG("AsyncRequest", "Received response:\n%s", std::string(response.begin(), response.end()).c_str());

            std::string response_str(response.begin(), response.end());
            callback_(Result::Success, std::make_shared<Response>(response_str));
        }
    } // namespace http
} // namespace tortoise