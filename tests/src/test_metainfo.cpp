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

	const std::array<std::uint8_t, 20> expected_info_hash = {
		0xca, 0xa9, 0x0f, 0x24,
		0xb7, 0xf3, 0xf4, 0xf2,
		0x6f, 0xe7, 0xbe, 0x81,
		0xef, 0x9a, 0x9c, 0xd3,
		0x87, 0xf4, 0xe9, 0x07};

	EXPECT_EQ(expected_info_hash, metainfo->GetInfoHash());
}

TEST(Metainfo, missing_announce_fails)
{
	auto data = bencode::Decode("d4:infod6:lengthi12345e4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, missing_info_fails)
{
	auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announcee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, missing_length_fails)
{
	auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announce4:infod4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, missing_name_fails)
{
	auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, missing_piece_length_fails)
{
	auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e4:name4:spam6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, missing_pieces_fails)
{
	auto data = bencode::Decode("d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e4:name4:spam12:piece lengthi16384eee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, info_with_both_length_and_files_fails)
{
	auto data = bencode::Decode("d8:announce19:http://tracker1.com13:announce-listll19:http://tracker1.com19:http://tracker2.com19:http://tracker3.comee8:comments46:Just a testing comment for my testing torrent.10:created by14:KTorrent 2.1.413:creation datei1182163222e4:infod6:lengthi5e5:filesld6:lengthi3184e4:pathl9:file1.txteed6:lengthi2878e4:pathl9:file2.txteed6:lengthi3412e4:pathl9:file3.txteed6:lengthi2804e4:pathl9:file4.txteee4:name8:root_dir12:piece lengthi262144e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, multi_file)
{
	auto data = bencode::Decode("d8:announce19:http://tracker1.com13:announce-listll19:http://tracker1.com19:http://tracker2.com19:http://tracker3.comee8:comments46:Just a testing comment for my testing torrent.10:created by14:KTorrent 2.1.413:creation datei1182163222e4:infod5:filesld6:lengthi3184e4:pathl9:file1.txteed6:lengthi2878e4:pathl9:file2.txteed6:lengthi3412e4:pathl9:file3.txteed6:lengthi2804e4:pathl9:file4.txteee4:name8:root_dir12:piece lengthi262144e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	auto metainfo = Metainfo::FromBencode(*data);
	ASSERT_NE(nullptr, metainfo);
	EXPECT_EQ("http://tracker1.com", metainfo->GetAnnounce());
	EXPECT_EQ("root_dir", metainfo->GetName());
	EXPECT_EQ(262144, metainfo->GetPieceLength());
	ASSERT_EQ(1, metainfo->GetPieces().size());
	EXPECT_EQ("aaaaaaaaaaaaaaaaaaaa", metainfo->GetPieces()[0]);
	auto file_info = std::get<Metainfo::MultiFile>(metainfo->GetFileInfo());
	ASSERT_EQ(4, file_info.files.size());
	EXPECT_EQ(3184, file_info.files[0].length);
	EXPECT_EQ("file1.txt", file_info.files[0].path[0]);
	EXPECT_EQ(2878, file_info.files[1].length);
	EXPECT_EQ("file2.txt", file_info.files[1].path[0]);
	EXPECT_EQ(3412, file_info.files[2].length);
	EXPECT_EQ("file3.txt", file_info.files[2].path[0]);
	EXPECT_EQ(2804, file_info.files[3].length);
	EXPECT_EQ("file4.txt", file_info.files[3].path[0]);

	const std::array<uint8_t, 20> expected_info_hash = {
		0x86, 0x19, 0x1C, 0xEC,
		0x81, 0xA5, 0xCA, 0x48,
		0x03, 0xBB, 0x82, 0xCA,
		0x40, 0xE4, 0xEB, 0xF6,
		0xFD, 0xE1, 0xA7, 0xD5};

	EXPECT_EQ(expected_info_hash, metainfo->GetInfoHash());
}