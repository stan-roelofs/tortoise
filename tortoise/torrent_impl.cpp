#include "torrent_impl.hpp"

#include <cassert>

#include "log.hpp"

namespace
{
    constexpr std::size_t DESIRED_PEERS = 20;
}

namespace tortoise
{
    bool TorrentHandle::IsValid() const
    {
        return !ptr_.expired();
    }

    Metainfo TorrentHandle::GetMetainfo() const
    {
        if (!IsValid())
            throw TorrentException("TorrentHandle is not valid");

        return *ptr_.lock()->GetMetainfo();
    }

    TorrentHandle::operator bool() const
    {
        return IsValid();
    }

    bool TorrentHandle::operator==(const TorrentHandle &other) const
    {
        return ptr_.lock() == other.ptr_.lock();
    }

    bool TorrentHandle::operator!=(const TorrentHandle &other) const
    {
        return !(*this == other);
    }

    Torrent::Torrent(const TorrentParameters &parameters, EventQueue &event_queue) : event_queue_(event_queue), metainfo_(std::make_shared<Metainfo>(parameters.metainfo)),
                                                                                     tracker_manager_(metainfo_->announce_list, std::bind(&Torrent::GetTrackerRequest, this)),
                                                                                     piece_manager_(metainfo_->pieces.size())
    {
        if (parameters.save_path.empty())
            throw TorrentException("save_path is empty"); // TODO set a default save path somewhere

        //
    }

    Torrent::~Torrent() = default;

    void Torrent::Process()
    {
        // TODO should each torrent have its own thread??
        if (tracker_manager_.Update())
        {
            const auto new_peers = tracker_manager_.GetPeers();
            for (const auto &peer : new_peers)
                AddPeer(peer);
        }

        ProcessPeers();
    }

    std::shared_ptr<const Metainfo> Torrent::GetMetainfo() const
    {
        return metainfo_;
    }

    bool Torrent::AddPeer(const PeerInfo &peer_info)
    {
        if (std::find(peer_queue_.begin(), peer_queue_.end(), peer_info) != peer_queue_.end())
            return false;
        if (std::find_if(peers_.begin(), peers_.end(), [&peer_info](const auto &peer)
                         { return peer.GetPeerInfo() == peer_info; }) != peers_.end())
            return false;

        peer_queue_.push_back(peer_info);
        return true;
    }

    void Torrent::ProcessPeers()
    {
        // Add new peers if we don't have enough and there are still peers in the queue
        // TODO send a tracker request if we need more peers
        while (peers_.size() < DESIRED_PEERS && !peer_queue_.empty())
        {
            PeerInfo peer_info = peer_queue_.front();
            peer_queue_.pop_front();

            LOG("Torrent", "Adding peer %s %u", peer_info.ip.c_str(), peer_info.port);
            Peer::Callbacks callbacks;
            callbacks.on_connect = std::bind(&Torrent::OnConnect, this, std::placeholders::_1);
            callbacks.on_choke = std::bind(&Torrent::OnChoked, this, std::placeholders::_1);
            callbacks.on_unchoke = std::bind(&Torrent::OnUnchoked, this, std::placeholders::_1);
            callbacks.on_interested = std::bind(&Torrent::OnInterested, this, std::placeholders::_1);
            callbacks.on_not_interested = std::bind(&Torrent::OnNotInterested, this, std::placeholders::_1);
            callbacks.on_new_have = std::bind(&Torrent::OnNewHave, this, std::placeholders::_1, std::placeholders::_2);
            callbacks.on_request = std::bind(&Torrent::OnRequest, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
            callbacks.on_piece = std::bind(&Torrent::OnPiece, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
            callbacks.on_cancel = std::bind(&Torrent::OnCancel, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);
            callbacks.on_port = std::bind(&Torrent::OnPort, this, std::placeholders::_1, std::placeholders::_2);
            peers_.emplace_back(peer_info, metainfo_, peer_id_, callbacks);

            if (event_queue_.EventEnabled(EventType::PeerStatusChanged))
                event_queue_.PushEvent(std::make_unique<PeerStatusChangedEvent>(TorrentHandle(shared_from_this()), peer_info.ip, peer_info.port, PeerStatus::Connecting));
        }

        auto it = peers_.begin();
        while (it != peers_.end())
        {
            if (!it->Process())
            {
                const auto &info = it->GetPeerInfo();
                (void)info;
                LOG("Torrent", "Peer %s %u finished", info.ip.c_str(), info.port);

                if (event_queue_.EventEnabled(EventType::PeerStatusChanged))
                    event_queue_.PushEvent(std::make_unique<PeerStatusChangedEvent>(TorrentHandle(shared_from_this()), info.ip, info.port, PeerStatus::Disconnected));

                it = peers_.erase(it);
                continue;
            }

            ++it;
        }
    }

    AnnounceParameters Torrent::GetTrackerRequest()
    {
        AnnounceParameters request(metainfo_->info_hash, peer_id_);
        return request; // TODO
    }

    void Torrent::OnConnect(Peer &peer)
    {
        if (event_queue_.EventEnabled(EventType::PeerStatusChanged))
            event_queue_.PushEvent(std::make_unique<PeerStatusChangedEvent>(TorrentHandle(shared_from_this()), peer.GetPeerInfo().ip, peer.GetPeerInfo().port, PeerStatus::Connected));
    }

    void Torrent::OnChoked(Peer &)
    {
    }
    void Torrent::OnUnchoked(Peer &)
    {
    }
    void Torrent::OnInterested(Peer &)
    {
    }
    void Torrent::OnNotInterested(Peer &)
    {
    }
    void Torrent::OnNewHave(Peer &, std::set<std::uint32_t> pieces)
    {
    }
    void Torrent::OnRequest(Peer &, std::uint32_t, std::uint32_t, std::uint32_t)
    {
    }
    void Torrent::OnPiece(Peer &, std::uint32_t, std::uint32_t, const ByteVector &)
    {
    }
    void Torrent::OnCancel(Peer &, std::uint32_t, std::uint32_t, std::uint32_t)
    {
    }
    void Torrent::OnPort(Peer &, std::uint16_t)
    {
    }
}