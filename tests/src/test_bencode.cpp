#include <gtest/gtest.h>

#include <tortoise/bencode.hpp>

using namespace tortoise;
using namespace bencode;

TEST(bencode, decode_string)
{
    const string_t str = "4:spam";
    EXPECT_EQ("spam", get<string_t>(*decode(str)));
}

TEST(bencode, decode_string_empty)
{
    const string_t str = "0:";
    EXPECT_EQ("", get<string_t>(*decode(str)));
}

TEST(bencode, decode_string_missing_data_throws)
{
    const string_t str = "4:spa";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_string_missing_colon_throws)
{
    const string_t str = "4spam";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_positive)
{
    const string_t str = "i42e";
    EXPECT_EQ(42, get<integer_t>(*decode(str)));
}

TEST(bencode, decode_integer_negative)
{
    const string_t str = "i-42e";
    EXPECT_EQ(-42, get<integer_t>(*decode(str)));
}

TEST(bencode, decode_integer_zero)
{
    const string_t str = "i0e";
    EXPECT_EQ(0, get<integer_t>(*decode(str)));
}

TEST(bencode, decode_integer_leading_zero_throws)
{
    const string_t str = "i03e";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_negative_throws)
{
    const string_t str = "i-03e";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_negative_zero_throws)
{
    const string_t str = "i-0e";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_no_end_throws)
{
    const string_t str = "i42";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_no_end_start_throws)
{
    const string_t str = "42";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_missing_data_throws)
{
    const string_t str = "ie";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_integer_invalid_character_throws)
{
    const string_t str = "i4.2e";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_list_empty)
{
    const string_t str = "le";
    EXPECT_EQ(list_t(), get<list_t>(*decode(str)));
}

TEST(bencode, decode_list)
{
    const string_t str = "l4:spam4:eggse";
    auto result = get<list_t>(*decode(str));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("spam", get<string_t>(*result[0]));
    EXPECT_EQ("eggs", get<string_t>(*result[1]));
}

TEST(bencode, decode_list_nested)
{
    const string_t str = "ll4:spam4:eggsele4:spam4:eggseee";
    auto result = get<list_t>(*decode(str));
    ASSERT_EQ(4u, result.size());

    auto result2 = get<list_t>(*result[0]);
    ASSERT_EQ(2u, result2.size());
    EXPECT_EQ("spam", get<string_t>(*result2[0]));
    EXPECT_EQ("eggs", get<string_t>(*result2[1]));

    EXPECT_EQ(list_t(), get<list_t>(*result[1]));
    EXPECT_EQ("spam", get<string_t>(*result[2]));
    EXPECT_EQ("eggs", get<string_t>(*result[3]));
}

TEST(bencode, decode_list_no_end_throws)
{
    const string_t str = "l";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, decode_dictionary_empty)
{
    const string_t str = "de";
    EXPECT_EQ(dictionary_t(), get<dictionary_t>(*decode(str)));
}

TEST(bencode, decode_dictionary)
{
    const string_t str = "d3:cow3:moo4:spam4:eggse";
    auto result = get<dictionary_t>(*decode(str));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("moo", get<string_t>(*result["cow"]));
    EXPECT_EQ("eggs", get<string_t>(*result["spam"]));
}

TEST(bencode, decode_dictionary_no_end_throws)
{
    const string_t str = "d";
    EXPECT_THROW(decode(str), BencodeException);
}

TEST(bencode, get_string)
{
    EXPECT_EQ("spam", get<string_t>(*decode("4:spam")));
}

TEST(bencode, get_string_invalid_type_throws)
{
    EXPECT_THROW(get<string_t>(*decode("i42e")), BencodeException);
}

TEST(bencode, get_integer)
{
    EXPECT_EQ(42, get<integer_t>(*decode("i42e")));
}

TEST(bencode, get_integer_data_invalid_type_throws)
{
    EXPECT_THROW(get<integer_t>(*decode("spam")), BencodeException);
}

TEST(bencode, get_list_data)
{
    auto result = get<list_t>(*decode("l4:spam4:eggse"));
    EXPECT_EQ("spam", get<string_t>(*result[0]));
    EXPECT_EQ("eggs", get<string_t>(*result[1]));
}

TEST(bencode, get_list_data_invalid_type_throws)
{
    EXPECT_THROW(get<list_t>(*decode("spam")), BencodeException);
}

TEST(bencode, get_dictionary_data)
{
    auto result = get<dictionary_t>(*decode("d3:cow3:moo4:spam4:eggse"));
    EXPECT_EQ("moo", get<string_t>(*result["cow"]));
    EXPECT_EQ("eggs", get<string_t>(*result["spam"]));
}

TEST(bencode, get_dictionary_data_invalid_type_throws)
{
    EXPECT_THROW(get<dictionary_t>(*decode("spam")), BencodeException);
}

TEST(bencode, get_optional)
{
    EXPECT_EQ("spam", get_optional<string_t>(*decode("4:spam")));
}

TEST(bencode, get_optional_none)
{
    EXPECT_EQ(std::nullopt, get_optional<string_t>(*decode("i42e")));
}

TEST(bencode, get_or)
{
    EXPECT_EQ("spam", get_or<string_t>(*decode("4:spam"), "eggs"));
}

TEST(bencode, get_or_default)
{
    EXPECT_EQ("eggs", get_or<string_t>(*decode("i42e"), "eggs"));
}