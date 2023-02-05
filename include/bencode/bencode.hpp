#ifndef __BENCODE_HPP__
#define __BENCODE_HPP__

#include <string>
#include <map>
#include <memory>
#include <vector>

namespace bencode
{
    class exception : public std::exception
    {
    public:
        exception(const std::string &msg) : msg_(msg) {}
        exception(const char *msg) : msg_(msg) {}
        const char *what() const noexcept override { return msg_.c_str(); }

    private:
        std::string msg_;
    };

    class decode_exception : public exception
    {
    public:
        decode_exception(const std::string &msg) : exception(msg) {}
        decode_exception(const char *msg) : exception(msg) {}

    private:
        std::string msg_;
    };

    using string = std::string;
    using string_view = std::string_view;
    using integer = std::uint64_t;

    class visitor;

    class data
    {
    public:
        virtual string encode() = 0;
        virtual void accept(visitor &v) = 0;
    };

    using list = std::vector<std::shared_ptr<data>>;
    using dictionary = std::map<string, std::shared_ptr<data>>;

    class visitor
    {
    public:
        virtual void visit(string &str) = 0;
        virtual void visit(integer &num) = 0;
        virtual void visit(list &lst) = 0;
        virtual void visit(dictionary &dct) = 0;
    };

    class string_data : public data
    {
    public:
        string_data(string str) : str_(std::move(str)) {}
        string encode() override { return str_; }
        void accept(visitor &v) override { v.visit(str_); }

    private:
        string str_;
    };

    class integer_data : public data
    {
    public:
        integer_data(integer num) : num_(num) {}
        string encode() override { return "i" + std::to_string(num_) + "e"; }
        void accept(visitor &v) override { v.visit(num_); }

    private:
        integer num_;
    };

    class list_data : public data
    {
    public:
        list_data(list lst) : lst_(std::move(lst)) {}
        string encode() override
        {
            string result = "l";
            for (auto &item : lst_)
                result += item->encode();
            result += "e";
            return result;
        }
        void accept(visitor &v) override { v.visit(lst_); }

    private:
        list lst_;
    };

    class dictionary_data : public data
    {
    public:
        dictionary_data(dictionary dct) : dct_(std::move(dct)) {}
        string encode() override
        {
            string result = "d";
            for (auto &item : dct_)
            {
                result += std::to_string(item.first.length()) + ":" + item.first;
                result += item.second->encode();
            }
            result += "e";
            return result;
        }
        void accept(visitor &v) override { v.visit(dct_); }

    private:
        dictionary dct_;
    };

    class type_exception : public exception
    {
    public:
        type_exception(const std::string &msg) : exception(msg) {}
        type_exception(const char *msg) : exception(msg) {}
    };

    template <class T>
    class getter : public visitor
    {
    public:
        getter(data &data) {}
        void visit(string &str) override
        {
            if constexpr (std::is_same_v<T, string>)
                result_ = str;
            else
                throw type_exception("Invalid type");
        }
        void visit(integer &num) override
        {
            if constexpr (std::is_same_v<T, integer>)
                result_ = num;
            else
                throw type_exception("Invalid type");
        }
        void visit(list &lst) override
        {
            if constexpr (std::is_same_v<T, list>)
                result_ = lst;
            else
                throw type_exception("Invalid type");
        }
        void visit(dictionary &dct) override
        {
            if constexpr (std::is_same_v<T, dictionary>)
                result_ = dct;
            else
                throw type_exception("Invalid type");
        }

        T result() const { return result_; }

    private:
        T result_;
    };

    template <class T>
    T get(data &data)
    {
        getter<T> g(data);
        data.accept(g);
        return g.result();
    }

    std::unique_ptr<data> decode(string str);
}

#endif