#include <tortoise/peer_id.hpp>

#include <random>
#include <limits>

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
        static_assert(TORTOISE_VERSION_MAJOR < 10, "TORTOISE_VERSION_MAJOR must be less than 10");
        static_assert(TORTOISE_VERSION_MINOR < 10, "TORTOISE_VERSION_MINOR must be less than 10");
        static_assert(TORTOISE_VERSION_PATCH < 100, "TORTOISE_VERSION_PATCH must be less than 100");
        peer_id.push_back(TORTOISE_VERSION_MAJOR);
        peer_id.push_back(TORTOISE_VERSION_MINOR);
        peer_id.push_back(TORTOISE_VERSION_PATCH / 10);
        peer_id.push_back(TORTOISE_VERSION_PATCH % 10);
        peer_id.push_back('-');
        for (size_t i = 8; i < 20; i++)
            peer_id.push_back(static_cast<char>(dist(gen)));

        return peer_id;
    }

    PeerId::PeerId() : peer_id_(GeneratePeerID())
    {
    }

    PeerId::~PeerId() = default;

    const std::string &PeerId::Get() const
    {
        return peer_id_;
    }
}