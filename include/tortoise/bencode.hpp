#ifndef TORTOISE_BENCODE_HPP
#define TORTOISE_BENCODE_HPP

#include <string>
#include <map>
#include <memory>
#include <optional>
#include <vector>

#include "exceptions.hpp"

namespace tortoise
{
    namespace bencode
    {
        using string_t = std::string;
        using string_view_t = std::string_view;
        using integer_t = std::uint64_t;

        class Visitor;

        class Data
        {
        public:
            virtual ~Data() = default;
            virtual string_t Encode() = 0;
            virtual void Accept(Visitor &v) = 0;
        };

        using list_t = std::vector<std::shared_ptr<Data>>;
        using dictionary_t = std::map<string_t, std::shared_ptr<Data>>;

        class Visitor
        {
        public:
            virtual void Visit(string_t &str) = 0;
            virtual void Visit(integer_t &num) = 0;
            virtual void Visit(list_t &lst) = 0;
            virtual void Visit(dictionary_t &dct) = 0;
        };

        template <class T>
        class GetValueVisitor : public Visitor
        {
        public:
            void Visit(string_t &str) override
            {
                if constexpr (std::is_same_v<T, string_t>)
                    result_ = str;
                else
                    throw BencodeException("Invalid type");
            }
            void Visit(integer_t &num) override
            {
                if constexpr (std::is_same_v<T, integer_t>)
                    result_ = num;
                else
                    throw BencodeException("Invalid type");
            }
            void Visit(list_t &lst) override
            {
                if constexpr (std::is_same_v<T, list_t>)
                    result_ = lst;
                else
                    throw BencodeException("Invalid type");
            }
            void Visit(dictionary_t &dct) override
            {
                if constexpr (std::is_same_v<T, dictionary_t>)
                    result_ = dct;
                else
                    throw BencodeException("Invalid type");
            }

            T result() const { return result_; }

        private:
            T result_;
        };

        template <class T>
        T get(Data &data)
        {
            GetValueVisitor<T> g;
            data.Accept(g);
            return g.result();
        }

        template <class T>
        std::optional<T> get_optional(Data &data)
        {
            try
            {
                return get<T>(data);
            }
            catch (BencodeException &)
            {
                return std::nullopt;
            }
        }

        template <class T>
        T get_or(Data &data, T def)
        {
            try
            {
                return get<T>(data);
            }
            catch (BencodeException &)
            {
                return def;
            }
        }

        /*!
         * \brief Decode a bencoded string into a Data object.
         * \param str The bencoded string.
         * \return A Data object.
         * \throws BencodeException if the string is not valid.
         */
        std::unique_ptr<Data> decode(string_t str);
    } // namespace bencode
} // namespace tortoise

#endif