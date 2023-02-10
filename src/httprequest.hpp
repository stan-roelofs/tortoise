#ifndef TORTOISE_HTTPREQUEST_HPP
#define TORTOISE_HTTPREQUEST_HPP

#include <map>
#include <string>

#include <tortoise/connection.hpp>

namespace tortoise
{
    // TODO should this even be a class? could just be a function
    class HTTPRequest
    {
    public:
        /*! \brief Creates a new HTTP request.
         *  \param url The URL to send the request to.
         *  \param params The parameters to send with the request.
         *  \return A new HTTP request which may be nullptr if the request could not be created.
         */
        static std::unique_ptr<HTTPRequest> Create(const std::string &url, const std::map<std::string, std::string> &params);

        ~HTTPRequest();

        //! \brief Sends the request and blocks until a response is received.
        bool Get(std::string &response);

    private:
        HTTPRequest(std::unique_ptr<Connection> connection, const std::string &request);

        std::unique_ptr<Connection> connection_;
        std::string request_;
        std::string response_;
    };
}

#endif