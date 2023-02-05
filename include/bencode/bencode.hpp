#ifndef __BENCODE_HPP__
#define __BENCODE_HPP__

#include <string>
#include <string_view>

namespace bencode
{
    std::string encode(std::string_view str);
    std::string decode(std::string_view str);
}

#endif