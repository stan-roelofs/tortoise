#ifndef TORTOISE_METADATA_HPP
#define TORTOISE_METADATA_HPP

#include <variant>

#include "bencode.hpp"

namespace tortoise
{
    class Metainfo
    {
    public:
        /*!
         * \brief Create metainfo from bencoded data.
         * \param data Bencoded data.
         * \return A new metainfo object if the data is valid, otherwise nullptr.
         */
        static std::unique_ptr<Metainfo> FromBencode(const bencode::Data &data);

        //! \brief The URL of the tracker.
        std::string GetAnnounce() const;

        //! \brief A UTF-8 encoded string which is the suggested name to save the file (or directory) as.
        std::string GetName() const;

        //! \brief The number of bytes in each piece the file is split into.
        std::uint32_t GetPieceLength() const;

        //! \brief A list of strings corresponding to the SHA1 hash values of each piece the file is split into.
        std::vector<std::string> GetPieces() const;

        struct SingleFile
        {
            std::uint32_t length;
        };

        struct MultiFile
        {
            struct File
            {
                //! \brief The length of the file in bytes.
                std::uint32_t length;
                //! \brief A list of UTF-8 encoded strings corresponding to subdirectory name, the last is the actual file name.
                std::vector<std::string> path;
            };

            std::vector<File> files;
        };

        std::variant<SingleFile, MultiFile> GetFileInfo() const;

    private:
        std::string announce_;
        std::string name_;
        std::uint32_t piece_length_;
        std::vector<std::string> pieces_;
        std::variant<SingleFile, MultiFile> file_info_;
    };
}

#endif