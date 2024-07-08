#ifndef TORTOISE_URL_HPP
#define TORTOISE_URL_HPP

#include <string>

#include <tortoise/exception.hpp>

namespace tortoise
{
    namespace network
    {
        class URLException : public Exception
        {
        public:
            URLException(const std::string &msg) : Exception(msg) {}
        };

        class URL
        {
        public:
            URL();

            /*! \param url The URL to parse.
             *  \throws URLException If the URL is invalid.
             */
            URL(const std::string &url);

            //! \returns The protocol of the URL, e.g. "http", "https" or "udp".
            const std::string &GetProtocol() const;

            //! \returns The host of the URL, e.g. "tracker.example.com".
            const std::string &GetHost() const;

            //! \returns The port of the URL, e.g. "80".
            const std::string &GetPort() const;

            //! \returns The path of the URL, e.g. "/announce".
            const std::string &GetPath() const;

            //! \returns The URL as a string
            std::string ToString() const;

            bool operator==(const URL &other) const;
            bool operator!=(const URL &other) const;

        private:
            std::string protocol_;
            std::string host_;
            std::string port_;
            std::string path_;
        };
    }
}

#endif