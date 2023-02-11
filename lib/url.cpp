#include <tortoise/url.hpp>

#include <tortoise/exceptions.hpp>

namespace tortoise
{
    URL::URL(const std::string &url)
    {
        std::size_t pos = url.find("://");
        if (pos == std::string::npos)
            throw URLException("Invalid URL, missing protocol");

        protocol_ = url.substr(0, pos);
        host_ = url.substr(pos + 3);

        const size_t path_start = host_.find("/");
        if (path_start != std::string::npos)
        {
            path_ = host_.substr(path_start);
            host_ = host_.substr(0, path_start);
        }

        const size_t port_start = host_.find(":");
        if (port_start != std::string::npos)
        {
            port_ = host_.substr(port_start + 1);
            host_ = host_.substr(0, port_start);
        }
    }

    const std::string &URL::GetProtocol() const
    {
        return protocol_;
    }

    const std::string &URL::GetHost() const
    {
        return host_;
    }

    const std::string &URL::GetPort() const
    {
        return port_;
    }

    const std::string &URL::GetPath() const
    {
        return path_;
    }
}