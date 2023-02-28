#include <gtest/gtest.h>

#include <tortoise/peer_id.hpp>
#include <tortoise/version.hpp>

using namespace tortoise;

TEST(PeerId, length_is_20)
{
    PeerId peer_id;
    ASSERT_EQ(peer_id.Get().size(), 20);
}

TEST(PeerId, client_information_is_correct)
{
	PeerId peer_id;
	ASSERT_EQ(peer_id.Get()[0], '-');
	ASSERT_EQ(peer_id.Get()[1], 'T');
	ASSERT_EQ(peer_id.Get()[2], 'R');
	ASSERT_EQ(peer_id.Get()[3], TORTOISE_VERSION_MAJOR);
	ASSERT_EQ(peer_id.Get()[4], TORTOISE_VERSION_MINOR);
	ASSERT_EQ(peer_id.Get()[5], TORTOISE_VERSION_PATCH / 10);
	ASSERT_EQ(peer_id.Get()[6], TORTOISE_VERSION_PATCH % 10);
	ASSERT_EQ(peer_id.Get()[7], '-');
}

TEST(PeerId, random_part_is_random)
{
	PeerId peer_id1;
	PeerId peer_id2;
	ASSERT_NE(peer_id1.Get().substr(8), peer_id2.Get().substr(8)); // There is a very very small chance that this test will fail, I guess it's okay.
}