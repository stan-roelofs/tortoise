#include <gtest/gtest.h>
#include <sstream>

#include "../tortoise/bencode.hpp"

using namespace tortoise;
using namespace bencode;

TEST(bencode, decode_string)
{
    const string_t str = "4:spam";
    std::istringstream iss(str);
    EXPECT_EQ("spam", Get<string_t>(*Decode(iss)));
}

TEST(bencode, decode_string_empty)
{
    const string_t str = "0:";
    std::istringstream iss(str);
    EXPECT_EQ("", Get<string_t>(*Decode(iss)));
}

TEST(bencode, decode_string_missing_data_throws)
{
    const string_t str = "5:spa";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_string_missing_colon_throws)
{
    const string_t str = "4spam";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_positive)
{
    const string_t str = "i42e";
    std::istringstream iss(str);
    EXPECT_EQ(42, Get<integer_t>(*Decode(iss)));
}

TEST(bencode, decode_integer_negative)
{
    const string_t str = "i-42e";
    std::istringstream iss(str);
    EXPECT_EQ(-42, Get<integer_t>(*Decode(iss)));
}

TEST(bencode, decode_integer_zero)
{
    const string_t str = "i0e";
    std::istringstream iss(str);
    EXPECT_EQ(0, Get<integer_t>(*Decode(iss)));
}

TEST(bencode, decode_integer_leading_zero_throws)
{
    const string_t str = "i03e";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_negative_throws)
{
    const string_t str = "i-03e";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_negative_zero_throws)
{
    const string_t str = "i-0e";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_no_end_throws)
{
    const string_t str = "i42";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_no_end_start_throws)
{
    const string_t str = "42";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_missing_data_throws)
{
    const string_t str = "ie";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_integer_invalid_character_throws)
{
    const string_t str = "i4.2e";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_list_empty)
{
    const string_t str = "le";
    std::istringstream iss(str);
    EXPECT_EQ(list_t(), Get<list_t>(*Decode(iss)));
}

TEST(bencode, decode_list)
{
    const string_t str = "l4:spam4:eggse";
    std::istringstream iss(str);
    auto result = Get<list_t>(*Decode(iss));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("spam", Get<string_t>(*result[0]));
    EXPECT_EQ("eggs", Get<string_t>(*result[1]));
}

TEST(bencode, decode_list_nested)
{
    const string_t str = "ll4:spam4:eggsele4:spam4:eggseee";
    std::istringstream iss(str);
    auto result = Get<list_t>(*Decode(iss));
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
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, decode_dictionary_empty)
{
    const string_t str = "de";
    std::istringstream iss(str);
    EXPECT_EQ(dictionary_t(), Get<dictionary_t>(*Decode(iss)));
}

TEST(bencode, decode_dictionary)
{
    const string_t str = "d3:cow3:moo4:spam4:eggse";
    std::istringstream iss(str);
    auto result = Get<dictionary_t>(*Decode(iss));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("moo", Get<string_t>(*result["cow"]));
    EXPECT_EQ("eggs", Get<string_t>(*result["spam"]));
}

TEST(bencode, decode_dictionary_no_end_throws)
{
    const string_t str = "d";
    std::istringstream iss(str);
    EXPECT_THROW(Decode(iss), BencodeException);
}

TEST(bencode, encode_string)
{
    const StringData str("spam");
    EXPECT_EQ("4:spam", str.Encode());
}

TEST(bencode, encode_integer)
{
    const IntegerData integer(42);
    EXPECT_EQ("i42e", integer.Encode());
}

TEST(bencode, encode_integer_zero)
{
    const IntegerData integer(0);
    EXPECT_EQ("i0e", integer.Encode());
}

TEST(bencode, encode_integer_negative_zero)
{
    const IntegerData integer(-0);
    EXPECT_EQ("i0e", integer.Encode());
}

TEST(bencode, encode_integer_negative)
{
    const IntegerData integer(-42);
    EXPECT_EQ("i-42e", integer.Encode());
}

TEST(bencode, encode_list)
{
    list_t list;
    list.push_back(std::make_shared<StringData>("spam"));
    list.push_back(std::make_shared<StringData>("eggs"));
    const ListData lst = ListData(move(list));
    EXPECT_EQ("l4:spam4:eggse", lst.Encode());
}

TEST(bencode, encode_list_empty)
{
    const ListData lst = ListData(list_t());
    EXPECT_EQ("le", lst.Encode());
}

TEST(bencode, encode_dictionary_empty)
{
    const DictionaryData dictionary = DictionaryData(dictionary_t());
    EXPECT_EQ("de", dictionary.Encode());
}

TEST(bencode, encode_dictionary_is_sorted)
{
    dictionary_t dictionary;
    dictionary["spam"] = std::make_shared<StringData>("eggs");
    dictionary["cow"] = std::make_shared<StringData>("moo");
    const DictionaryData dictionaryData = DictionaryData(move(dictionary));
    EXPECT_EQ("d3:cow3:moo4:spam4:eggse", dictionaryData.Encode());
}

TEST(bencode, get_string)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_EQ("spam", Get<string_t>(*Decode(iss)));
}

TEST(bencode, get_string_invalid_type_throws)
{
    std::string str("i42e");
    std::istringstream iss(str);
    EXPECT_THROW(Get<string_t>(*Decode(iss)), BencodeException);
}

TEST(bencode, get_integer)
{
    std::string str("i42e");
    std::istringstream iss(str);
    EXPECT_EQ(42, Get<integer_t>(*Decode(iss)));
}

TEST(bencode, get_integer_data_invalid_type_throws)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_THROW(Get<integer_t>(*Decode(iss)), BencodeException);
}

TEST(bencode, get_list_data)
{
    std::string str("l4:spam4:eggse");
    std::istringstream iss(str);
    auto result = Get<list_t>(*Decode(iss));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("spam", Get<string_t>(*result[0]));
    EXPECT_EQ("eggs", Get<string_t>(*result[1]));
}

TEST(bencode, get_list_data_invalid_type_throws)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_THROW(Get<list_t>(*Decode(iss)), BencodeException);
}

TEST(bencode, get_dictionary_data)
{
    std::string str("d3:cow3:moo4:spam4:eggse");
    std::istringstream iss(str);
    auto result = Get<dictionary_t>(*Decode(iss));
    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("moo", Get<string_t>(*result["cow"]));
    EXPECT_EQ("eggs", Get<string_t>(*result["spam"]));
}

TEST(bencode, get_dictionary_data_invalid_type_throws)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_THROW(Get<dictionary_t>(*Decode(iss)), BencodeException);
}

TEST(bencode, check_type_string_true)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_TRUE(CheckType<string_t>(*Decode(iss)));
}

TEST(bencode, check_type_string_false)
{
    std::string str("i42e");
    std::istringstream iss(str);
    EXPECT_FALSE(CheckType<string_t>(*Decode(iss)));
}

TEST(bencode, check_type_integer_true)
{
    std::string str("i42e");
    std::istringstream iss(str);
    EXPECT_TRUE(CheckType<integer_t>(*Decode(iss)));
}

TEST(bencode, check_type_integer_false)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_FALSE(CheckType<integer_t>(*Decode(iss)));
}

TEST(bencode, check_type_list_true)
{
    std::string str("l4:spam4:eggse");
    std::istringstream iss(str);
    EXPECT_TRUE(CheckType<list_t>(*Decode(iss)));
}

TEST(bencode, check_type_list_false)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_FALSE(CheckType<list_t>(*Decode(iss)));
}

TEST(bencode, check_type_dictionary_true)
{
    std::string str("d3:cow3:moo4:spam4:eggse");
    std::istringstream iss(str);
    EXPECT_TRUE(CheckType<dictionary_t>(*Decode(iss)));
}

TEST(bencode, check_type_dictionary_false)
{
    std::string str("4:spam");
    std::istringstream iss(str);
    EXPECT_FALSE(CheckType<dictionary_t>(*Decode(iss)));
}