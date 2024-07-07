#ifndef TORTOISE_METADATA_HPP
#define TORTOISE_METADATA_HPP

#include <array>
#include <string>
#include <vector>

namespace tortoise
{
    struct Metainfo
    {
        /*! \brief A list of lists of tracker URLs as described in BEP 12. If there is no announce - list key,
         * this is a list of one list containing the announce key.
         */
        std::vector<std::vector<std::string>> announce_list;

        //! \brief The SHA1 hash of the bencoded value of the info key.
        std::array<std::uint8_t, 20> info_hash;

        //! \brief (optional) The creation time of the torrent, in standard UNIX epoch format.
        uint64_t creation_date;

        //! \brief (optional) Free-form textual comments of the author.
        std::string comment;

        //! \brief (optional) Name and version of the program used to create the .torrent.
        std::string created_by;

        //! \brief (optional) The string encoding format used to generate the pieces part of the info dictionary.
        std::string encoding;

        //! \brief A UTF-8 encoded string which is the suggested name to save the file (or directory) as.
        std::string name;

        //! \brief The number of bytes in each piece the file is split into.
        std::uint32_t piece_length;

        //! \brief A list of strings corresponding to the SHA1 hash values of each piece the file is split into.
        std::vector<std::string> pieces;

        struct File
        {
            //! \brief The length of the file in bytes.
            std::uint32_t length;

            //! \brief A list of UTF-8 encoded strings corresponding to subdirectory name, the last is the actual file name.
            std::vector<std::string> path;
        };

        //! \brief Either a single file or a list of files.
        std::vector<File> files;
    };
}

#endif