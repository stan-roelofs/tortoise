#ifndef TORTOISE_METADATA_HPP
#define TORTOISE_METADATA_HPP

#include <array>
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
        const std::string &GetAnnounce() const;

        //! \brief (optional) A list of lists of tracker URLs, for backups if a tracker fails or if different tiers of trackers are used.
        const std::vector<std::vector<std::string>> &GetAnnounceList() const;

        //! \brief (optional) The creation time of the torrent, in standard UNIX epoch format.
        uint64_t GetCreationDate() const;

        //! \brief (optional) Free-form textual comments of the author.
        const std::string &GetComment() const;

        //! \brief (optional) Name and version of the program used to create the .torrent.
        const std::string &GetCreatedBy() const;

        //! \brief (optional) The string encoding format used to generate the pieces part of the info dictionary.
        const std::string &GetEncoding() const;

        //! \brief A UTF-8 encoded string which is the suggested name to save the file (or directory) as.
        const std::string &GetName() const;

        //! \brief The number of bytes in each piece the file is split into.
        std::uint32_t GetPieceLength() const;

        //! \brief A list of strings corresponding to the SHA1 hash values of each piece the file is split into.
        const std::vector<std::string> &GetPieces() const;

        //! \brief The SHA1 hash of the bencoded value of the info key.
        const std::array<std::uint8_t, 20> &GetInfoHash() const;

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
        std::vector<std::vector<std::string>> announce_list_;
        uint64_t creation_date_;
        std::string comment_;
        std::string created_by_;
        std::string encoding_;
        std::string name_;
        std::uint32_t piece_length_;
        std::vector<std::string> pieces_;
        std::variant<SingleFile, MultiFile> file_info_;
        std::array<std::uint8_t, 20> info_hash_;
    };
}

#endif