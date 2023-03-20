#ifndef TORTOISE_HTTP_REQUEST_HPP
#define TORTOISE_HTTP_REQUEST_HPP

#include <functional>
#include <map>
#include <memory>
#include <thread>

#include <tortoise/socket.hpp>
#include <tortoise/url.hpp>

#include "response.hpp"

namespace tortoise
{
    namespace http
    {
        //! \brief A class for sending HTTP GET requests asynchronously.
        class AsyncRequest
        {
        public:
            AsyncRequest(const URL &url);
            AsyncRequest(const AsyncRequest &) = delete;
            AsyncRequest(AsyncRequest &&) = delete;
            AsyncRequest &operator=(const AsyncRequest &) = delete;
            AsyncRequest &operator=(AsyncRequest &&) = delete;
            ~AsyncRequest();

            void AddParameter(const std::string &key, const std::string &value);

            enum class Result
            {
                Success,
                Failure,
                Cancelled // TODO
            };

            void SetTimeout(unsigned int timeout);

            bool Get(std::function<void(Result result, std::shared_ptr<Response> response)> callback);

        private:
            std::string CreateRequest(const URL &url, const std::map<std::string, std::string> &params);
            static void ThreadFunc(AsyncRequest *request);

            void SendRequest();

            std::map<std::string, std::string> params_;
            std::function<void(Result result, std::shared_ptr<Response> response)> callback_;
            std::string request_;
            URL url_;
            unsigned int timeout_;

            std::thread thread_;
        };
    }
}

#endif