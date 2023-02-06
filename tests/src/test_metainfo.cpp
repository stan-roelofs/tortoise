#include <gtest/gtest.h>

#include <tortoise/metainfo.hpp>

using namespace tortoise;

TEST(Metainfo, single_file)
{
    auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
    auto metainfo = Metainfo::FromBencode(*data);
    ASSERT_NE(nullptr, metainfo);
    EXPECT_EQ("http://tracker.example.com/announce", metainfo->GetAnnounce());
    EXPECT_EQ("spam", metainfo->GetName());
    EXPECT_EQ(16384, metainfo->GetPieceLength());
    ASSERT_EQ(1, metainfo->GetPieces().size());
    EXPECT_EQ("aaaaaaaaaaaaaaaaaaaa", metainfo->GetPieces()[0]);
    auto file_info = std::get<Metainfo::SingleFile>(metainfo->GetFileInfo());
    EXPECT_EQ(12345, file_info.length);
}

TEST(Metainfo, multi_file)
{
	auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announce4:infod5:filesld6:lengthi12345e4:pathl4:spam4:eggsee6:lengthi67890e4:pathl4:spam4:hammeee4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_NE(nullptr, metainfo);
	EXPECT_EQ("http://tracker.example.com/announce", metainfo->GetAnnounce());
	EXPECT_EQ("spam", metainfo->GetName());
	EXPECT_EQ(16384, metainfo->GetPieceLength());
	ASSERT_EQ(1, metainfo->GetPieces().size());
	EXPECT_EQ("aaaaaaaaaaaaaaaaaaaa", metainfo->GetPieces()[0]);
	auto file_info = std::get<Metainfo::MultiFile>(metainfo->GetFileInfo());
	ASSERT_EQ(2, file_info.files.size());
	EXPECT_EQ(12345, file_info.files[0].length);
	EXPECT_EQ("spam/egg", file_info.files[0].path);
	EXPECT_EQ(67890, file_info.files[1].length);
	EXPECT_EQ("spam/ham", file_info.files[1].path);
}