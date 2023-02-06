#ifndef TORTOISE_METADATA_HPP
#define TORTOISE_METADATA_HPP

#include <variant>

#include "bencode.hpp"

namespace tortoise
{
    class Metainfo
    {
    public:
        Metainfo(bencode::data &data);

        //! \brief The URL of the tracker.
        std::string GetAnnounce() const;

        //! \brief A UTF-8 encoded string which is the suggested name to save the file (or directory) as.
        std::string GetName() const;

        //! \brief The number of bytes in each piece the file is split into.
        std::uint64_t GetPieceLength() const;

        //! \brief A list of strings corresponding to the SHA1 hash values of each piece the file is split into.
        std::vector<std::string> GetPieces() const;

        struct SingleFile
        {
            std::uint64_t length;
        };

        struct MultiFile
        {
            struct File
            {
                //! \brief The length of the file in bytes.
                std::uint64_t length;
                //! \brief A list of UTF-8 encoded strings corresponding to subdirectory name, the last is the actual file name.
                std::string path;
            };

            std::vector<File> files;
        };

        std::variant<SingleFile, MultiFile> GetFileInfo() const;
    };
}

#endif