#include <tortoise/bencode.hpp>

namespace tortoise
{
    namespace bencode
    {
        class StringData : public Data
        {
        public:
            StringData(string_t str) : str_(std::move(str)) {}
            string_t Encode() override { return str_; }
            void Accept(Visitor &v) override { v.Visit(str_); }

        private:
            string_t str_;
        };

        class IntegerData : public Data
        {
        public:
            IntegerData(integer_t num) : num_(num) {}
            string_t Encode() override { return "i" + std::to_string(num_) + "e"; }
            void Accept(Visitor &v) override { v.Visit(num_); }

        private:
            integer_t num_;
        };

        class ListData : public Data
        {
        public:
            ListData(list_t lst) : lst_(std::move(lst)) {}
            string_t Encode() override
            {
                string_t result = "l";
                for (auto &item : lst_)
                    result += item->Encode();
                result += "e";
                return result;
            }
            void Accept(Visitor &v) override { v.Visit(lst_); }

        private:
            list_t lst_;
        };

        class DictionaryData : public Data
        {
        public:
            DictionaryData(dictionary_t dct) : dct_(std::move(dct)) {}
            string_t Encode() override
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
            void Accept(Visitor &v) override { v.Visit(dct_); }

        private:
            dictionary_t dct_;
        };

        std::unique_ptr<Data> decode_internal(string_t &str);

        string_t decode_string(string_t &str)
        {
            const auto colon = str.find(':');
            if (colon == string_t::npos)
                throw BencodeException("Invalid string: no colon");

            std::size_t pos = 0;
            const auto length = std::stoull(str.substr(0, colon), &pos, 10);
            if (pos != colon)
                throw BencodeException("Invalid string: invalid length");

            if (length > str.length() - colon - 1)
                throw BencodeException("Invalid string: not enough data");

            string_t result = str.substr(colon + 1, length);
            str = str.substr(colon + 1 + length);
            return result;
        }

        integer_t decode_integer(string_t &str)
        {
            if (str.size() <= 2)
                throw BencodeException("Invalid integer: no data");

            if (str.front() != 'i')
                throw BencodeException("Invalid integer: no initial i");

            const auto end = str.find('e');
            if (end == string_t::npos)
                throw BencodeException("Invalid integer: no trailing e");

            const auto data = str.substr(1, end - 1);
            if ((data[0] == '0' && data.length() > 1) || (data[0] == '-' && data[1] == '0'))
                throw BencodeException("Invalid integer: leading zero");

            std::size_t pos = 0;
            const auto value = std::stoll(std::string(data), &pos, 10);
            if (pos != data.length())
                throw BencodeException("Invalid integer");

            return value;
        }

        list_t decode_list(string_t &str)
        {
            if (str.size() < 2)
                throw BencodeException("Invalid list: no data");

            if (str.front() != 'l')
                throw BencodeException("Invalid list: no initial l");

            list_t result;
            str = str.substr(1);
            while (!str.empty() && str.front() != 'e')
                result.push_back(std::move(decode_internal(str)));

            if (str.empty())
                throw BencodeException("Invalid list: no trailing e");

            str = str.substr(1);

            return result;
        }

        dictionary_t decode_dictionary(string_t &str)
        {
            if (str.size() < 2)
                throw BencodeException("Invalid decode_dictionary: no data");

            if (str.front() != 'd')
                throw BencodeException("Invalid decode_dictionary: no initial d");

            dictionary_t result;
            str = str.substr(1);
            while (!str.empty() && str.front() != 'e')
            {
                const auto key = decode_string(str);
                result[key] = std::move(decode_internal(str));
            }

            if (str.empty())
                throw BencodeException("Invalid decode_dictionary: no trailing e");

            str = str.substr(1);

            return result;
        }

        std::unique_ptr<Data> decode_internal(string_t &str)
        {
            switch (str[0])
            {
            case 'i':
                return std::make_unique<IntegerData>(decode_integer(str));
            case 'l':
                return std::make_unique<ListData>(decode_list(str));
            case 'd':
                return std::make_unique<DictionaryData>(decode_dictionary(str));
            }

            return std::make_unique<StringData>(decode_string(str));
        }

        std::unique_ptr<Data> decode(string_t str)
        {
            return decode_internal(str);
        }
    } // namespace bencode
} // namespace tortoise