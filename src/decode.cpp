#include <bencode/bencode.hpp>

namespace bencode
{
    std::unique_ptr<data> decode_internal(string &str);

    string decode_string(string &str)
    {
        const auto colon = str.find(':');
        if (colon == string::npos)
            throw decode_exception("Invalid string: no colon");

        std::size_t pos = 0;
        const auto length = std::stoull(str.substr(0, colon), &pos, 10);
        if (pos != colon)
            throw decode_exception("Invalid string: invalid length");

        if (length > str.length() - colon - 1)
            throw decode_exception("Invalid string: not enough data");

        string result = str.substr(colon + 1, length);
        str = str.substr(colon + 1 + length);
        return result;
    }

    integer decode_integer(string &str)
    {
        if (str.size() <= 2)
            throw decode_exception("Invalid integer: no data");

        if (str.front() != 'i')
            throw decode_exception("Invalid integer: no initial i");

        const auto end = str.find('e');
        if (end == string::npos)
            throw decode_exception("Invalid integer: no trailing e");

        const auto data = str.substr(1, end - 1);
        if ((data[0] == '0' && data.length() > 1) || (data[0] == '-' && data[1] == '0'))
            throw decode_exception("Invalid integer: leading zero");

        std::size_t pos = 0;
        const auto value = std::stoll(std::string(data), &pos, 10);
        if (pos != data.length())
            throw decode_exception("Invalid integer");

        return value;
    }

    list decode_list(string &str)
    {
        if (str.size() < 2)
            throw decode_exception("Invalid list: no data");

        if (str.front() != 'l')
            throw decode_exception("Invalid list: no initial l");

        list result;
        str = str.substr(1);
        while (!str.empty() && str.front() != 'e')
            result.push_back(std::move(decode_internal(str)));

        if (str.empty())
            throw decode_exception("Invalid list: no trailing e");

        str = str.substr(1);

        return result;
    }

    dictionary decode_dictionary(string &str)
    {
        if (str.size() < 2)
            throw decode_exception("Invalid decode_dictionary: no data");

        if (str.front() != 'd')
            throw decode_exception("Invalid decode_dictionary: no initial d");

        dictionary result;
        str = str.substr(1);
        while (!str.empty() && str.front() != 'e')
        {
            const auto key = decode_string(str);
            result[key] = std::move(decode_internal(str));
        }

        if (str.empty())
            throw decode_exception("Invalid decode_dictionary: no trailing e");

        str = str.substr(1);

        return result;
    }

    std::unique_ptr<data> decode_internal(string &str)
    {
        switch (str[0])
        {
        case 'i':
            return std::make_unique<integer_data>(decode_integer(str));
        case 'l':
            return std::make_unique<list_data>(decode_list(str));
        case 'd':
            return std::make_unique<dictionary_data>(decode_dictionary(str));
        default:
            return std::make_unique<string_data>(decode_string(str));
        }

        return nullptr;
    }

    std::unique_ptr<data> decode(string str)
    {
        return decode_internal(str);
    }
} // namespace bencode