#ifndef TORTOISE_HTTP_REQUEST_HPP
#define TORTOISE_HTTP_REQUEST_HPP

#include <map>
#include <memory>

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
            /*! \param url The URL to send the request to.
             *  \param params The parameters to send with the request.
             *  \throws HTTPException If the URL is invalid.
             */
            AsyncRequest(const URL &url, const std::map<std::string, std::string> &params = {});

            /*! \brief Processes the request. The expectation is that this is called repeatedly until it returns true.
             *  \returns True if the request is complete.
             */
            bool Process();

            //! \returns True if the request is complete.
            bool Done() const;

            /*! \brief Returns the response to the request.
             *  \returns The response to the request, or nullptr if the request is not complete.
             */
            std::shared_ptr<Response> GetResponse() const;

        private:
            void CreateRequest(const URL &url, const std::map<std::string, std::string> &params);

            std::shared_ptr<Response> response_;
            std::string request_;
            std::string response_buffer_;
            Socket socket_;

            enum class State
            {
                Connecting,
                Sending,
                Receiving,
                Done
            };

            int bytes_sent_;
            State state_;
        };
    }
}

#endif