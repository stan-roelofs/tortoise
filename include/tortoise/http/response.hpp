#ifndef TORTOISE_HTTP_RESPONSE_HPP
#define TORTOISE_HTTP_RESPONSE_HPP

#include <map>
#include <optional>
#include <string>

namespace tortoise
{
    namespace http
    {
        class Response
        {
        public:
            /*! \brief Parses the response.
             *  \param response The response to parse.
             *  \throws HTTPException If the response is invalid.
             */
            Response(const std::string &response);

            //! \returns True if the status code indicates success.
            bool Success() const;

            //! \returns The HTTP specification to which the server has tried to make the message comply.
            const std::string &GetHTTPVersion() const;

            //! \returns A three-digit number indicating the result of the request.
            std::uint16_t GetStatusCode() const;

            //! \returns A short text description of the status code.
            const std::string &GetStatusText() const;

            //! \returns the HTTP headers.
            const std::string &GetHeaders() const;

            //! \returns The body of the response.
            const std::string &GetBody() const;

        private:
            std::string http_version;
            std::uint16_t status_code;
            std::string status_text;
            std::string body_;
            std::string headers_;

            const std::string response_;
        };
    }
}

#endif