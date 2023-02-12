#ifndef TORTOISE_BENCODE_HPP
#define TORTOISE_BENCODE_HPP

#include <istream>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <tortoise/exceptions.hpp>

namespace tortoise
{
    namespace bencode
    {
        using string_t = std::string;
        using string_view_t = std::string_view;
        using integer_t = std::int64_t;

        class Visitor;

        class Data
        {
        public:
            virtual ~Data() = default;
            virtual string_t Encode() const = 0;
            virtual void Accept(Visitor &v) const = 0;
        };

        using list_t = std::vector<std::shared_ptr<Data>>;
        using dictionary_t = std::map<string_t, std::shared_ptr<Data>>;

        class StringData : public Data
        {
        public:
            StringData(string_t str);
            string_t Encode() const override;
            void Accept(Visitor &v) const override;
            const string_t &GetString() const;

        private:
            string_t str_;
        };

        class IntegerData : public Data
        {
        public:
            IntegerData(integer_t num);
            string_t Encode() const override;
            void Accept(Visitor &v) const override;
            const integer_t &GetInteger() const;

        private:
            integer_t num_;
        };

        class ListData : public Data
        {
        public:
            ListData(list_t lst);
            string_t Encode() const override;
            void Accept(Visitor &v) const override;
            const list_t &GetList() const;

        private:
            list_t lst_;
        };

        class DictionaryData : public Data
        {
        public:
            DictionaryData(dictionary_t dct);
            string_t Encode() const override;
            void Accept(Visitor &v) const override;
            const dictionary_t &GetDictionary() const;

        private:
            dictionary_t dct_;
        };

        class Visitor
        {
        public:
            virtual void Visit(const StringData &str) = 0;
            virtual void Visit(const IntegerData &num) = 0;
            virtual void Visit(const ListData &lst) = 0;
            virtual void Visit(const DictionaryData &dct) = 0;
        };

        template <class T>
        class GetValueVisitor : public Visitor
        {
        public:
            GetValueVisitor() : result_(nullptr) {}
            void Visit(const StringData &str) override
            {
                if constexpr (std::is_same_v<T, string_t>)
                    result_ = &str.GetString();
            }
            void Visit(const IntegerData &num) override
            {
                if constexpr (std::is_same_v<T, integer_t>)
                    result_ = &num.GetInteger();
            }
            void Visit(const ListData &lst) override
            {
                if constexpr (std::is_same_v<T, list_t>)
                    result_ = &lst.GetList();
            }
            void Visit(const DictionaryData &dct) override
            {
                if constexpr (std::is_same_v<T, dictionary_t>)
                    result_ = &dct.GetDictionary();
            }

            const T &result() const
            {
                if (!result_)
                    throw BencodeException("No value found");
                return *result_;
            }

        private:
            const T *result_;
        };

        template <class T>
        const T &Get(const Data &data)
        {
            GetValueVisitor<T> g;
            data.Accept(g);
            return g.result();
        }

        template <class T>
        class CheckTypeVisitor : public Visitor
        {
        public:
            CheckTypeVisitor() : result_(false) {}
            void Visit(const StringData &) override
            {
                if constexpr (std::is_same_v<T, string_t>)
                    result_ = true;
            }
            void Visit(const IntegerData &) override
            {
                if constexpr (std::is_same_v<T, integer_t>)
                    result_ = true;
            }
            void Visit(const ListData &) override
            {
                if constexpr (std::is_same_v<T, list_t>)
                    result_ = true;
            }
            void Visit(const DictionaryData &) override
            {
                if constexpr (std::is_same_v<T, dictionary_t>)
                    result_ = true;
            }

            bool result() const
            {
                return result_;
            }

        private:
            bool result_;
        };

        template <class T>
        bool CheckType(const Data &data)
        {
            CheckTypeVisitor<T> v;
            data.Accept(v);
            return v.result();
        }

        /*!
         * \brief Decode a bencoded string into a Data object.
         * \param stream The stream to read from.
         * \return A Data object.
         * \throws BencodeException if the string is not valid.
         */
        std::unique_ptr<Data> Decode(std::istream &stream);
    } // namespace bencode
} // namespace tortoise

#endif