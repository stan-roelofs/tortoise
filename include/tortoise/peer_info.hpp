#ifndef TORTOISE_PEER_INFO_HPP
#define TORTOISE_PEER_INFO_HPP

#include <cstdint>
#include <optional>
#include <string>

#include "peer_id.hpp"

namespace tortoise
{
	struct PeerInfo
	{
		PeerInfo(const std::string& ip, std::uint16_t port) : ip(ip), port(port), ip_port(ip + ":" + std::to_string(port)) {}
		std::optional<PeerId> peer_id; // The 20-byte self-selected peer id of the peer.
		std::string ip;                // The peer's IP address either IPv6 (hexed) or IPv4 (dotted quad) or DNS name (string).
		std::uint16_t port;
		std::string ip_port; // The peer's IP address and port in the format "ip:port".

		std::string ToString() const { return ip_port; }

		bool operator==(const PeerInfo& other) const
		{
			return ip == other.ip && port == other.port;
		}

		bool operator!=(const PeerInfo& other) const
		{
			return !(*this == other);
		}

		bool operator<(const PeerInfo& other) const
		{
			return ip_port < other.ip_port;
		}
	};

	enum class PeerStatus
	{
		Connecting,
		Connected,
		Disconnected,
	};
}

#endif