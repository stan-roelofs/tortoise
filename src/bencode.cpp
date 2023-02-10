#include <tortoise/bencode.hpp>

#include <stdexcept>
#include <sstream>

namespace tortoise
{
    namespace bencode
    {
        StringData::StringData(string_t str) : str_(std::move(str)) {}
        string_t StringData::Encode() const
        {
            return std::to_string(str_.length()) + ":" + str_;
        }
        void StringData::Accept(Visitor &v) const
        {
            v.Visit(*this);
        }
        const std::string &StringData::GetString() const
        {
            return str_;
        }

        IntegerData::IntegerData(integer_t num) : num_(num) {}
        string_t IntegerData::Encode() const
        {
            return "i" + std::to_string(num_) + "e";
        }
        void IntegerData::Accept(Visitor &v) const
        {
            v.Visit(*this);
        }
        const integer_t &IntegerData::GetInteger() const
        {
            return num_;
        }

        ListData::ListData(list_t lst) : lst_(std::move(lst)) {}
        string_t ListData::Encode() const
        {
            string_t result = "l";
            for (const auto &item : lst_)
            {
                result += item->Encode();
            }
            result += "e";
            return result;
        }
        void ListData::Accept(Visitor &v) const
        {
            v.Visit(*this);
        }
        const list_t &ListData::GetList() const
        {
            return lst_;
        }

        DictionaryData::DictionaryData(dictionary_t dct) : dct_(std::move(dct)) {}
        string_t DictionaryData::Encode() const
        {
            string_t result = "d";
            for (const auto &item : dct_)
            {
                result += std::to_string(item.first.length()) + ":" + item.first;
                result += item.second->Encode();
            }
            result += "e";
            return result;
        }
        void DictionaryData::Accept(Visitor &v) const
        {
            v.Visit(*this);
        }
        const dictionary_t &DictionaryData::GetDictionary() const
        {
            return dct_;
        }

        static std::unique_ptr<Data> decode_internal(std::istream &stream);

        static std::uint8_t get_next(std::istream &stream)
        {
            if (stream.eof())
                throw BencodeException("Unexpected end of stream");

            return static_cast<std::uint8_t>(stream.get());
        }

        static std::uint64_t get_integer(std::istream &stream, std::uint8_t terminator, std::uint8_t prev_char = 0)
        {
            std::uint64_t result = 0;
            std::uint8_t c = 0;

            if (prev_char >= '0' && prev_char <= '9')
                result = prev_char - '0';

            while (true)
            {
                c = get_next(stream);
                if (c == terminator)
                    break;
                if (c < '0' || c > '9')
                    throw BencodeException("Found unexpected character while parsing integer: " + std::to_string(c));
                result = result * 10 + (c - '0');
            }

            return result;
        }

        static string_t decode_string(std::istream &stream)
        {
            const std::uint64_t length = get_integer(stream, ':');
            std::stringstream ss;
            for (std::uint64_t i = 0; i < length; ++i)
                ss << get_next(stream);
            return ss.str();
        }

        integer_t decode_integer(std::istream &stream)
        {
            uint8_t c = get_next(stream);
            if (c != 'i')
                throw BencodeException("Invalid integer: no initial i");

            c = get_next(stream);
            bool negative = false;
            if (c == '-')
            {
                negative = true;
                c = get_next(stream);
            }
            if (c == '0')
            {
                if (negative)
                    throw BencodeException("Invalid integer: leading zero");
                c = get_next(stream);
                if (c != 'e')
                    throw BencodeException("Invalid integer: leading zero");
                return 0;
            }

            return get_integer(stream, 'e', c) * static_cast<integer_t>((negative ? -1 : 1));
        }

        list_t decode_list(std::istream &stream)
        {
            std::uint8_t c = get_next(stream);
            if (c != 'l')
                throw BencodeException("Invalid list: no initial l");

            list_t result;
            while (!stream.eof() && stream.peek() != 'e')
                result.push_back(std::move(decode_internal(stream)));

            c = get_next(stream);
            if (c != 'e')
                throw BencodeException("Invalid list: no trailing e");

            return result;
        }

        dictionary_t decode_dictionary(std::istream &stream)
        {
            std::uint8_t c = get_next(stream);
            if (c != 'd')
                throw BencodeException("Invalid dictionary: no initial l");

            dictionary_t result;
            while (!stream.eof() && stream.peek() != 'e')
            {
                const string_t key = decode_string(stream);
                result[key] = std::move(decode_internal(stream));
            }

            c = get_next(stream);
            if (c != 'e')
                throw BencodeException("Invalid dictionary: no trailing e");

            return result;
        }

        std::unique_ptr<Data> decode_internal(std::istream &stream)
        {
            if (stream.eof())
                throw BencodeException("Unexpected end of stream");

            switch (stream.peek())
            {
            case 'i':
                return std::make_unique<IntegerData>(decode_integer(stream));
            case 'l':
                return std::make_unique<ListData>(decode_list(stream));
            case 'd':
                return std::make_unique<DictionaryData>(decode_dictionary(stream));
            }

            return std::make_unique<StringData>(decode_string(stream));
        }

        std::unique_ptr<Data> Decode(std::istream &stream)
        {
            return decode_internal(stream);
        }
    } // namespace bencode
} // namespace tortoise