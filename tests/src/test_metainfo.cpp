#include <gtest/gtest.h>

#include <tortoise/load_torrent.hpp>
#include <tortoise/metainfo.hpp>

using namespace tortoise;

TEST(Metainfo, single_file_without_optional_fields)
{
	std::string str("d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
	ASSERT_NE(nullptr, metainfo);
	EXPECT_EQ("http://tracker.example.com/announce", metainfo->announce_list.front().front());
	EXPECT_EQ("spam", metainfo->name);
	EXPECT_EQ(16384, metainfo->piece_length);
	ASSERT_EQ(1, metainfo->pieces.size());
	EXPECT_EQ("aaaaaaaaaaaaaaaaaaaa", metainfo->pieces[0]);
	ASSERT_EQ(1u, metainfo->files.size());
	auto file_info = metainfo->files.at(0);
	EXPECT_EQ(12345, file_info.length);

	const std::array<std::uint8_t, 20> expected_info_hash = {
		0xca, 0xa9, 0x0f, 0x24,
		0xb7, 0xf3, 0xf4, 0xf2,
		0x6f, 0xe7, 0xbe, 0x81,
		0xef, 0x9a, 0x9c, 0xd3,
		0x87, 0xf4, 0xe9, 0x07};

	EXPECT_EQ(expected_info_hash, metainfo->info_hash);
}
TEST(Metainfo, missing_announce_fails)
{
	std::string str("d4:infod6:lengthi12345e4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee");
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
	ASSERT_EQ(nullptr, metainfo);
}

TEST(Metainfo, missing_info_fails)
{
	std::string str = "d8:announce35:http://tracker.example.com/announcee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
}

TEST(Metainfo, missing_length_fails)
{
	std::string str = "d8:announce35:http://tracker.example.com/announce4:infod4:name4:spam12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
}

TEST(Metainfo, missing_name_fails)
{
	std::string str = "d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e12:piece lengthi16384e6:pieces20:aaaaaaaaaaaaaaaaaaaaee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
}

TEST(Metainfo, missing_piece_length_fails)
{
	std::string str = "d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e4:name4:spam6:pieces20:aaaaaaaaaaaaaaaaaaaaee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
}

TEST(Metainfo, missing_pieces_fails)
{
	std::string str = "d8:announce35:http://tracker.example.com/announce4:infod6:lengthi12345e4:name4:spam12:piece lengthi16384eee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
}

TEST(Metainfo, info_with_both_length_and_files_fails)
{
	std::string str = "d8:announce19:http://tracker1.com13:announce-listll19:http://tracker1.com19:http://tracker2.com19:http://tracker3.comee8:comments46:Just a testing comment for my testing torrent.10:created by14:KTorrent 2.1.413:creation datei1182163222e4:infod6:lengthi5e5:filesld6:lengthi3184e4:pathl9:file1.txteed6:lengthi2878e4:pathl9:file2.txteed6:lengthi3412e4:pathl9:file3.txteed6:lengthi2804e4:pathl9:file4.txteee4:name8:root_dir12:piece lengthi262144e6:pieces20:aaaaaaaaaaaaaaaaaaaaee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
}

TEST(Metainfo, multi_file_with_optional_fields)
{
	std::string str = "d8:announce19:http://tracker1.com13:announce-listll19:http://tracker1.com19:http://tracker2.com19:http://tracker3.comee7:comment46:Just a testing comment for my testing torrent.10:created by14:KTorrent 2.1.413:creation datei1182163222e8:encoding4:test4:infod5:filesld6:lengthi3184e4:pathl9:file1.txteed6:lengthi2878e4:pathl9:file2.txteed6:lengthi3412e4:pathl9:file3.txteed6:lengthi2804e4:pathl9:file4.txteee4:name8:root_dir12:piece lengthi262144e6:pieces20:aaaaaaaaaaaaaaaaaaaaee";
	std::istringstream iss(str);
	auto metainfo = LoadTorrent(iss);
	ASSERT_NE(nullptr, metainfo);
	auto announce_list = metainfo->announce_list;
	ASSERT_EQ(1u, announce_list.size());
	ASSERT_EQ(3u, announce_list[0].size());
	EXPECT_EQ("http://tracker1.com", announce_list[0][0]);
	EXPECT_EQ("http://tracker2.com", announce_list[0][1]);
	EXPECT_EQ("http://tracker3.com", announce_list[0][2]);
	EXPECT_EQ(1182163222, metainfo->creation_date);
	EXPECT_EQ("Just a testing comment for my testing torrent.", metainfo->comment);
	EXPECT_EQ("KTorrent 2.1.4", metainfo->created_by);
	EXPECT_EQ("test", metainfo->encoding);
	EXPECT_EQ("root_dir", metainfo->name);
	EXPECT_EQ(262144, metainfo->piece_length);
	ASSERT_EQ(1, metainfo->pieces.size());
	EXPECT_EQ("aaaaaaaaaaaaaaaaaaaa", metainfo->pieces[0]);
	auto file_info = metainfo->files;
	ASSERT_EQ(4, file_info.size());
	EXPECT_EQ(3184, file_info[0].length);
	EXPECT_EQ("file1.txt", file_info[0].path[0]);
	EXPECT_EQ(2878, file_info[1].length);
	EXPECT_EQ("file2.txt", file_info[1].path[0]);
	EXPECT_EQ(3412, file_info[2].length);
	EXPECT_EQ("file3.txt", file_info[2].path[0]);
	EXPECT_EQ(2804, file_info[3].length);
	EXPECT_EQ("file4.txt", file_info[3].path[0]);

	const std::array<uint8_t, 20> expected_info_hash = {
		0x86, 0x19, 0x1C, 0xEC,
		0x81, 0xA5, 0xCA, 0x48,
		0x03, 0xBB, 0x82, 0xCA,
		0x40, 0xE4, 0xEB, 0xF6,
		0xFD, 0xE1, 0xA7, 0xD5};

	EXPECT_EQ(expected_info_hash, metainfo->info_hash);
}