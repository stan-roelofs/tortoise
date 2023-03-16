#include <tortoise/load_torrent.hpp>

#include <filesystem>
#include <fstream>

namespace tortoise
{
    std::unique_ptr<Metainfo> LoadTorrentFile(const std::string &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file.is_open())
            return nullptr;

        std::unique_ptr<bencode::Data> data;
        try
        {
            data = bencode::Decode(file);
        }
        catch (const BencodeException &)
        {
            return nullptr;
        }

        return Metainfo::FromBencode(*data);
    }
}