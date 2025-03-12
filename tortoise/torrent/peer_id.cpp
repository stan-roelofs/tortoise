#include <tortoise/peer_info.hpp>

#include <random>
#include <limits>

#include <tortoise/exception.hpp>
#include <tortoise/version.hpp>

namespace tortoise
{
    std::string GeneratePeerID()
    {
        std::random_device r;
        std::mt19937 gen(r());
        std::uniform_int_distribution<int> dist(std::numeric_limits<char>::min(), std::numeric_limits<char>::max());

        std::string peer_id;
        peer_id.push_back('-');
        peer_id.push_back('T');
        peer_id.push_back('R');
        static_assert(VERSION_MAJOR < 10, "VERSION_MAJOR must be less than 10");
        static_assert(VERSION_MINOR < 10, "VERSION_MINOR must be less than 10");
        static_assert(VERSION_PATCH < 100, "VERSION_PATCH must be less than 100");
        peer_id.push_back(VERSION_MAJOR + '0');
        peer_id.push_back(VERSION_MINOR + '0');
        peer_id.push_back((VERSION_PATCH / 10) + '0');
        peer_id.push_back((VERSION_PATCH % 10) + '0');
        peer_id.push_back('-');
        for (size_t i = 8; i < 20; i++)
            peer_id.push_back(static_cast<char>(dist(gen)));

        return peer_id;
    }

    PeerId PeerId::FromString(const std::string &peer_id)
    {
        if (peer_id.size() != 20)
            throw InvalidArgumentException("Peer id must be 20 bytes long.");

        return PeerId(peer_id);
    }

    PeerId::PeerId() : peer_id_(GeneratePeerID())
    {
    }

    PeerId::PeerId(const std::string &peer_id) : peer_id_(peer_id)
    {
    }

    PeerId::~PeerId() = default;

    const std::string &PeerId::Get() const
    {
        return peer_id_;
    }

    bool PeerId::operator==(const PeerId &other) const
    {
        return peer_id_ == other.peer_id_;
    }

    bool PeerId::operator!=(const PeerId &other) const
    {
        return !(*this == other);
    }
}