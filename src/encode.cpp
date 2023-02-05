#include <bencode/bencode.hpp>

namespace bencode
{
    std::string encode(std::string_view str)
    {
        return std::string(str);
    }
} // namespace bencode