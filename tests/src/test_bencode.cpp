#include <gtest/gtest.h>

#include <tortoise/bencode/bencode.hpp>

using namespace tortoise;
using namespace bencode;

TEST(bencode, decode_string)
{
    const string_t str = "4:spam";
    EXPECT_EQ("spam", Get<string_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_string_empty)
{
    const string_t str = "0:";
    EXPECT_EQ("", Get<string_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_string_missing_data_throws)
{
    const string_t str = "4:spa";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_string_missing_colon_throws)
{
    const string_t str = "4spam";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_positive)
{
    const string_t str = "i42e";
    EXPECT_EQ(42, Get<integer_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_integer_negative)
{
    const string_t str = "i-42e";
    EXPECT_EQ(-42, Get<integer_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_integer_zero)
{
    const string_t str = "i0e";
    EXPECT_EQ(0, Get<integer_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_integer_leading_zero_throws)
{
    const string_t str = "i03e";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_negative_throws)
{
    const string_t str = "i-03e";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_negative_zero_throws)
{
    const string_t str = "i-0e";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_no_end_throws)
{
    const string_t str = "i42";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_no_end_start_throws)
{
    const string_t str = "42";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_missing_data_throws)
{
    const string_t str = "ie";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_integer_invalid_character_throws)
{
    const string_t str = "i4.2e";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_list_empty)
{
    const string_t str = "le";
    EXPECT_EQ(list_t(), Get<list_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_list)
{
    const string_t str = "l4:spam4:eggse";
    auto result = Get<list_t>(*Decode(str.c_str()));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("spam", Get<string_t>(*result[0]));
    EXPECT_EQ("eggs", Get<string_t>(*result[1]));
}

TEST(bencode, decode_list_nested)
{
    const string_t str = "ll4:spam4:eggsele4:spam4:eggseee";
    auto result = Get<list_t>(*Decode(str.c_str()));
    ASSERT_EQ(4u, result.size());

    auto result2 = Get<list_t>(*result[0]);
    ASSERT_EQ(2u, result2.size());
    EXPECT_EQ("spam", Get<string_t>(*result2[0]));
    EXPECT_EQ("eggs", Get<string_t>(*result2[1]));

    EXPECT_EQ(list_t(), Get<list_t>(*result[1]));
    EXPECT_EQ("spam", Get<string_t>(*result[2]));
    EXPECT_EQ("eggs", Get<string_t>(*result[3]));
}

TEST(bencode, decode_list_no_end_throws)
{
    const string_t str = "l";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, decode_dictionary_empty)
{
    const string_t str = "de";
    EXPECT_EQ(dictionary_t(), Get<dictionary_t>(*Decode(str.c_str())));
}

TEST(bencode, decode_dictionary)
{
    const string_t str = "d3:cow3:moo4:spam4:eggse";
    auto result = Get<dictionary_t>(*Decode(str.c_str()));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("moo", Get<string_t>(*result["cow"]));
    EXPECT_EQ("eggs", Get<string_t>(*result["spam"]));
}

TEST(bencode, decode_dictionary_no_end_throws)
{
    const string_t str = "d";
    EXPECT_THROW(Decode(str.c_str()), BencodeException);
}

TEST(bencode, get_string)
{
    EXPECT_EQ("spam", Get<string_t>(*Decode("4:spam")));
}

TEST(bencode, get_string_invalid_type_throws)
{
    EXPECT_THROW(Get<string_t>(*Decode("i42e")), BencodeException);
}

TEST(bencode, get_integer)
{
    EXPECT_EQ(42, Get<integer_t>(*Decode("i42e")));
}

TEST(bencode, get_integer_data_invalid_type_throws)
{
    EXPECT_THROW(Get<integer_t>(*Decode("spam")), BencodeException);
}

TEST(bencode, get_list_data)
{
    auto result = Get<list_t>(*Decode("l4:spam4:eggse"));
    EXPECT_EQ("spam", Get<string_t>(*result[0]));
    EXPECT_EQ("eggs", Get<string_t>(*result[1]));
}

TEST(bencode, get_list_data_invalid_type_throws)
{
    EXPECT_THROW(Get<list_t>(*Decode("spam")), BencodeException);
}

TEST(bencode, get_dictionary_data)
{
    auto result = Get<dictionary_t>(*Decode("d3:cow3:moo4:spam4:eggse"));
    EXPECT_EQ("moo", Get<string_t>(*result["cow"]));
    EXPECT_EQ("eggs", Get<string_t>(*result["spam"]));
}

TEST(bencode, get_dictionary_data_invalid_type_throws)
{
    EXPECT_THROW(Get<dictionary_t>(*Decode("spam")), BencodeException);
}

TEST(bencode, get_safe)
{
    EXPECT_EQ("spam", GetSafe<string_t>(*Decode("4:spam")));
}

TEST(bencode, get_safe_none)
{
    EXPECT_EQ(std::nullopt, GetSafe<string_t>(*Decode("i42e")));
}