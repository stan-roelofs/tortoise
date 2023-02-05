#include <gtest/gtest.h>

#include <bencode/bencode.hpp>

using namespace bencode;

TEST(Decode, string)
{
    const std::string str = "4:spam";
    EXPECT_EQ("spam", get<string>(*decode(str)));
}

TEST(Decode, string_empty)
{
    const std::string str = "0:";
    EXPECT_EQ("", get<string>(*decode(str)));
}

TEST(Decode, string_missing_data_throws)
{
    const std::string str = "4:spa";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, string_missing_colon_throws)
{
    const std::string str = "4spam";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_positive)
{
    const std::string str = "i42e";
    EXPECT_EQ(42, get<integer>(*decode(str)));
}

TEST(Decode, integer_negative)
{
    const std::string str = "i-42e";
    EXPECT_EQ(-42, get<integer>(*decode(str)));
}

TEST(Decode, integer_zero)
{
    const std::string str = "i0e";
    EXPECT_EQ(0, get<integer>(*decode(str)));
}

TEST(Decode, integer_leading_zero_throws)
{
    const std::string str = "i03e";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_negative_throws)
{
    const std::string str = "i-03e";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_negative_zero_throws)
{
    const std::string str = "i-0e";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_no_end_throws)
{
    const std::string str = "i42";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_no_end_start_throws)
{
    const std::string str = "42";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_missing_data_throws)
{
    const std::string str = "ie";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, integer_invalid_character_throws)
{
    const std::string str = "i4.2e";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, list_empty)
{
    const std::string str = "le";
    EXPECT_EQ(list(), get<list>(*decode(str)));
}

TEST(Decode, list)
{
    const std::string str = "l4:spam4:eggse";
    auto result = get<list>(*decode(str));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("spam", get<string>(*result[0]));
    EXPECT_EQ("eggs", get<string>(*result[1]));
}

TEST(Decode, list_nested)
{
    const std::string str = "ll4:spam4:eggsele4:spam4:eggseee";
    auto result = get<list>(*decode(str));
    ASSERT_EQ(4u, result.size());

    auto result2 = get<list>(*result[0]);
    ASSERT_EQ(2u, result2.size());
    EXPECT_EQ("spam", get<string>(*result2[0]));
    EXPECT_EQ("eggs", get<string>(*result2[1]));

    EXPECT_EQ(list(), get<list>(*result[1]));
    EXPECT_EQ("spam", get<string>(*result[2]));
    EXPECT_EQ("eggs", get<string>(*result[3]));
}

TEST(Decode, list_no_end_throws)
{
    const std::string str = "l";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Decode, dictionary_empty)
{
    const std::string str = "de";
    EXPECT_EQ(dictionary(), get<dictionary>(*decode(str)));
}

TEST(Decode, dictionary)
{
    const std::string str = "d3:cow3:moo4:spam4:eggse";
    auto result = get<dictionary>(*decode(str));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("moo", get<string>(*result["cow"]));
    EXPECT_EQ("eggs", get<string>(*result["spam"]));
}

TEST(Decode, dictionary_no_end_throws)
{
    const std::string str = "d";
    EXPECT_THROW(decode(str), decode_exception);
}

TEST(Get, string_data)
{
    string_data data("spam");
    EXPECT_EQ("spam", get<string>(data));
}

TEST(Get, string_data_invalid_type_throws)
{
    string_data data("spam");
    EXPECT_THROW(get<integer>(data), type_exception);
}

TEST(Get, integer_data)
{
    integer_data data(42);
    EXPECT_EQ(42, get<integer>(data));
}

TEST(Get, integer_data_invalid_type_throws)
{
    integer_data data(42);
    EXPECT_THROW(get<string>(data), type_exception);
}

TEST(Get, list_data)
{
    list data;
    data.push_back(std::make_shared<string_data>("spam"));
    data.push_back(std::make_shared<string_data>("eggs"));
    list_data data2(data);

    auto result = get<list>(data2);
    EXPECT_EQ(data, result);
}

TEST(Get, list_data_invalid_type_throws)
{
    list data;
    list_data data2(data);
    EXPECT_THROW(get<string>(data2), type_exception);
}

TEST(Get, dictionary_data)
{
    dictionary data;
    data["spam"] = std::make_shared<string_data>("eggs");
    dictionary_data data2(data);

    auto result = get<dictionary>(data2);
    EXPECT_EQ(data, result);
}

TEST(Get, dictionary_data_invalid_type_throws)
{
    dictionary data;
    dictionary_data data2(data);
    EXPECT_THROW(get<string>(data2), type_exception);
}