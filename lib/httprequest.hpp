#ifndef TORTOISE_HTTPREQUEST_HPP
#define TORTOISE_HTTPREQUEST_HPP

#include <map>
#include <string>

#include <tortoise/socket.hpp>
#include <tortoise/url.hpp>

namespace tortoise
{
    // TODO should this even be a class? could just be a function
    class HTTPRequest
    {
    public:
        /*! \param url The URL to send the request to.
         *  \param params The parameters to send with the request.
         *  \throws HTTPRequestException If the URL is invalid.
         */
        HTTPRequest(const URL &url, const std::map<std::string, std::string> &params);

        ~HTTPRequest();

        /*! \brief Sends the request and blocks until a response is received.
         *  \param socket The socket to use for the request.
         *  \returns The body of the HTTP response.
         *  \throws HTTPRequestException If the request failed.
         */
        std::string Get(Socket &socket) const;

    private:
        static std::string GetBodyFromResponse(const std::string &response);

        std::string request_;
    };
}

#endif