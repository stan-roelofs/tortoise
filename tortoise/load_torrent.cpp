#include <tortoise/load_torrent.hpp>

#include <filesystem>
#include <fstream>

#include "bencode.hpp"
#include "sha1.hpp"

namespace tortoise
{
    static std::unique_ptr<Metainfo> FromBencode(const bencode::Data &data)
    {
        try
        {
            const bencode::dictionary_t &dct = bencode::Get<bencode::dictionary_t>(data);

            std::unique_ptr<Metainfo> metainfo = std::make_unique<Metainfo>();

            // Calculate the info hash.
            try
            {
                const bencode::string_t &info_str = dct.at("info")->Encode();
                SHA1 sha1(info_str.c_str(), info_str.length());
                metainfo->info_hash = sha1.GetHash();
            }
            catch (SHA1::HashException &)
            {
                return nullptr;
            }

            // If the "announce-list" key is present, we will use it instead of the "announce" key.
            if (dct.find("announce-list") != dct.end())
            {
                const bencode::list_t &announce_list = bencode::Get<bencode::list_t>(*dct.at("announce-list"));
                for (const auto &announce_list_item : announce_list)
                {
                    const bencode::list_t &tier = bencode::Get<bencode::list_t>(*announce_list_item);
                    metainfo->announce_list.emplace_back();
                    for (const auto &tier_item : tier)
                        metainfo->announce_list.back().push_back(bencode::Get<bencode::string_t>(*tier_item));
                }
            }
            else
            {
                metainfo->announce_list.push_back(std::vector<std::string>());
                metainfo->announce_list.front().push_back(bencode::Get<bencode::string_t>(*dct.at("announce")));
            }

            if (dct.find("creation date") != dct.end())
                metainfo->creation_date = static_cast<uint64_t>(bencode::Get<bencode::integer_t>(*dct.at("creation date")));

            if (dct.find("comment") != dct.end())
                metainfo->comment = bencode::Get<bencode::string_t>(*dct.at("comment"));

            if (dct.find("created by") != dct.end())
                metainfo->created_by = bencode::Get<bencode::string_t>(*dct.at("created by"));

            if (dct.find("encoding") != dct.end())
                metainfo->encoding = bencode::Get<bencode::string_t>(*dct.at("encoding"));

            const bencode::dictionary_t &info = bencode::Get<bencode::dictionary_t>(*dct.at("info"));

            metainfo->name = bencode::Get<bencode::string_t>(*info.at("name"));
            metainfo->piece_length = static_cast<uint32_t>(bencode::Get<bencode::integer_t>(*info.at("piece length")));
            const std::string &pieces = bencode::Get<bencode::string_t>(*info.at("pieces"));

            if (pieces.length() % 20 != 0)
                return nullptr;

            for (std::size_t i = 0; i < pieces.size(); i += 20)
                metainfo->pieces.push_back(pieces.substr(i, 20));

            // These fields are mutually exclusive.
            if (info.find("files") != info.end() && info.find("length") != info.end())
                return nullptr;

            metainfo->files = {};
            if (info.find("length") != info.end())
                metainfo->files.push_back(Metainfo::File{static_cast<std::uint32_t>(bencode::Get<bencode::integer_t>(*info.at("length"))), {""}});
            else if (info.find("files") != info.end())
            {
                const bencode::list_t &files = bencode::Get<bencode::list_t>(*info.at("files"));
                for (const auto &file : files)
                {
                    const bencode::dictionary_t &file_dct = bencode::Get<bencode::dictionary_t>(*file);
                    Metainfo::File multi_file;
                    multi_file.length = static_cast<std::uint32_t>(bencode::Get<bencode::integer_t>(*file_dct.at("length")));

                    const bencode::list_t &path_pieces = bencode::Get<bencode::list_t>(*file_dct.at("path"));
                    for (const auto &path_piece : path_pieces)
                        multi_file.path.push_back(bencode::Get<bencode::string_t>(*path_piece));
                    metainfo->files.push_back(multi_file);
                }
            }
            else
                return nullptr;

            return metainfo;
        }
        catch (const bencode::BencodeException &)
        {
            return nullptr;
        }
        catch (const std::out_of_range &)
        {
            return nullptr;
        }
    }

    std::unique_ptr<Metainfo> LoadTorrent(const std::string &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return nullptr;

        return LoadTorrent(file);
    }

    std::unique_ptr<Metainfo> LoadTorrent(std::istream &stream)
    {
        std::unique_ptr<bencode::Data> data;
        try
        {
            data = bencode::Decode(stream);
        }
        catch (const bencode::BencodeException &)
        {
            return nullptr;
        }

        return FromBencode(*data);
    }
}