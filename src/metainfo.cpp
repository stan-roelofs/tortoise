#include <tortoise/metainfo.hpp>

#include <stdexcept>

namespace tortoise
{
    std::unique_ptr<Metainfo> Metainfo::FromBencode(const bencode::Data &data)
    {
        try
        {
            std::unique_ptr<Metainfo> metainfo = std::make_unique<Metainfo>();
            const bencode::dictionary_t dct = bencode::Get<bencode::dictionary_t>(data);
            metainfo->announce_ = bencode::Get<bencode::string_t>(*dct.at("announce"));

            const bencode::dictionary_t info = bencode::Get<bencode::dictionary_t>(*dct.at("info"));

            metainfo->name_ = bencode::Get<bencode::string_t>(*info.at("name"));
            metainfo->piece_length_ = bencode::Get<bencode::integer_t>(*info.at("piece length"));
            std::string pieces = bencode::Get<bencode::string_t>(*info.at("pieces"));

            if (pieces.length() % 20 != 0)
                return nullptr;

            for (std::size_t i = 0; i < pieces.size(); i += 20)
                metainfo->pieces_.push_back(pieces.substr(i, 20));

            std::optional<bencode::integer_t> length = bencode::GetSafe<bencode::integer_t>(*info.at("length"));
            if (length)
                metainfo->file_info_ = SingleFile{*length};
            else
            {
                std::optional<bencode::list_t> files = bencode::GetSafe<bencode::list_t>(*info.at("files"));
                if (!files)
                    return nullptr;

                MultiFile multi_file;
                for (const auto &file : *files)
                {
                    const bencode::dictionary_t file_dct = bencode::Get<bencode::dictionary_t>(*file);
                    MultiFile::File multi_file_file;
                    multi_file_file.length = bencode::Get<bencode::integer_t>(*file_dct.at("length"));
                    multi_file_file.path = bencode::Get<bencode::string_t>(*file_dct.at("path"));
                    multi_file.files.push_back(std::move(multi_file_file));
                }
                metainfo->file_info_ = std::move(multi_file);
            }

            return metainfo;
        }
        catch (const BencodeException &)
        {
            return nullptr;
        }
		catch (const std::out_of_range&)
		{
			return nullptr;
		}
    }

    std::string Metainfo::GetAnnounce() const
    {
        return announce_;
    }

    std::string Metainfo::GetName() const
    {
        return name_;
    }

    std::uint64_t Metainfo::GetPieceLength() const
    {
        return piece_length_;
    }

    std::vector<std::string> Metainfo::GetPieces() const
    {
        return pieces_;
    }

    std::variant<Metainfo::SingleFile, Metainfo::MultiFile> Metainfo::GetFileInfo() const
    {
        return file_info_;
    }
} // namespace tortoise