#include <tortoise/metainfo.hpp>

#include <stdexcept>

#include "sha1.hpp"

namespace tortoise
{
    std::unique_ptr<Metainfo> Metainfo::FromBencode(const bencode::Data &data)
    {
        try
        {
            const bencode::dictionary_t &dct = bencode::Get<bencode::dictionary_t>(data);

            std::unique_ptr<Metainfo> metainfo;
            // Calculate the info hash.
            const bencode::string_t &info_str = dct.at("info")->Encode();
            try
            {
                SHA1 sha1(info_str.c_str(), info_str.length());
                metainfo.reset(new Metainfo(sha1.GetHash()));
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
                    metainfo->announce_list_.emplace_back();
                    for (const auto &tier_item : tier)
                        metainfo->announce_list_.back().push_back(bencode::Get<bencode::string_t>(*tier_item));
                }
            }
            else
            {
                metainfo->announce_list_.push_back(std::vector<URL>());
                metainfo->announce_list_.front().push_back(bencode::Get<bencode::string_t>(*dct.at("announce")));
            }

            if (dct.find("creation date") != dct.end())
                metainfo->creation_date_ = static_cast<uint64_t>(bencode::Get<bencode::integer_t>(*dct.at("creation date")));

            if (dct.find("comment") != dct.end())
                metainfo->comment_ = bencode::Get<bencode::string_t>(*dct.at("comment"));

            if (dct.find("created by") != dct.end())
                metainfo->created_by_ = bencode::Get<bencode::string_t>(*dct.at("created by"));

            if (dct.find("encoding") != dct.end())
                metainfo->encoding_ = bencode::Get<bencode::string_t>(*dct.at("encoding"));

            const bencode::dictionary_t &info = bencode::Get<bencode::dictionary_t>(*dct.at("info"));

            metainfo->name_ = bencode::Get<bencode::string_t>(*info.at("name"));
            metainfo->piece_length_ = static_cast<uint32_t>(bencode::Get<bencode::integer_t>(*info.at("piece length")));
            const std::string &pieces = bencode::Get<bencode::string_t>(*info.at("pieces"));

            if (pieces.length() % 20 != 0)
                return nullptr;

            for (std::size_t i = 0; i < pieces.size(); i += 20)
                metainfo->pieces_.push_back(pieces.substr(i, 20));

            // These fields are mutually exclusive.
            if (info.find("files") != info.end() && info.find("length") != info.end())
                return nullptr;

            if (info.find("length") != info.end())
                metainfo->file_info_ = SingleFile{static_cast<std::uint32_t>(bencode::Get<bencode::integer_t>(*info.at("length")))};
            else if (info.find("files") != info.end())
            {
                MultiFile multi_file;
                const bencode::list_t &files = bencode::Get<bencode::list_t>(*info.at("files"));
                for (const auto &file : files)
                {
                    const bencode::dictionary_t &file_dct = bencode::Get<bencode::dictionary_t>(*file);
                    MultiFile::File multi_file_file;
                    multi_file_file.length = static_cast<std::uint32_t>(bencode::Get<bencode::integer_t>(*file_dct.at("length")));

                    const bencode::list_t &path_pieces = bencode::Get<bencode::list_t>(*file_dct.at("path"));
                    for (const auto &path_piece : path_pieces)
                        multi_file_file.path.push_back(bencode::Get<bencode::string_t>(*path_piece));
                    multi_file.files.push_back(std::move(multi_file_file));
                }
                metainfo->file_info_ = std::move(multi_file);
            }
            else
                return nullptr;

            return metainfo;
        }
        catch (const BencodeException &)
        {
            return nullptr;
        }
        catch (const std::out_of_range &)
        {
            return nullptr;
        }
    }

    Metainfo::Metainfo(const SHA1Hash &hash) : info_hash_(hash),
                                               creation_date_(0),
                                               piece_length_(0)
    {
    }

    const std::string &Metainfo::GetName() const
    {
        return name_;
    }

    const std::vector<std::vector<URL>> &Metainfo::GetAnnounceList() const
    {
        return announce_list_;
    }

    uint64_t Metainfo::GetCreationDate() const
    {
        return creation_date_;
    }

    const std::string &Metainfo::GetComment() const
    {
        return comment_;
    }

    const std::string &Metainfo::GetCreatedBy() const
    {
        return created_by_;
    }

    const std::string &Metainfo::GetEncoding() const
    {
        return encoding_;
    }

    std::uint32_t Metainfo::GetPieceLength() const
    {
        return piece_length_;
    }

    const std::vector<std::string> &Metainfo::GetPieces() const
    {
        return pieces_;
    }

    std::variant<Metainfo::SingleFile, Metainfo::MultiFile> Metainfo::GetFileInfo() const
    {
        return file_info_;
    }

    const SHA1Hash &Metainfo::GetInfoHash() const
    {
        return info_hash_;
    }
} // namespace tortoise