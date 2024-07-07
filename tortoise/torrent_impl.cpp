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
            throw InvalidHandleException("TorrentHandle is not valid");

        return *ptr_.lock()->GetMetainfo();
    }

    void TorrentHandle::StartDownload()
    {
        if (!IsValid())
            throw InvalidHandleException("TorrentHandle is not valid");

        ptr_.lock()->Start();
    }

    void TorrentHandle::StopDownload()
    {
        if (!IsValid())
            throw InvalidHandleException("TorrentHandle is not valid");

        ptr_.lock()->Stop();
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

    Torrent::Torrent(const TorrentParameters &parameters, Torrent::PeerInfoProvider &peer_info_provider, EventQueue &event_queue) : running_(false),
                                                                                                                                    peer_info_provider_(peer_info_provider),
                                                                                                                                    event_queue_(event_queue),
                                                                                                                                    metainfo_(std::make_shared<Metainfo>(parameters.metainfo)),
                                                                                                                                    piece_manager_(metainfo_->pieces.size())
    {
        if (parameters.save_path.empty())
            throw Exception("save_path is empty"); // TODO set a default save path somewhere

        peer_info_provider_.RequestPeers(*this, std::bind(&Torrent::AddPeer, this, std::placeholders::_1));

        //
    }

    Torrent::~Torrent()
    {
        peer_info_provider_.CancelRequest(*this);

        Stop();
    }

    void Torrent::Run(Torrent &torrent)
    {
        while (true)
        {
            {
                std::lock_guard lock(torrent.mutex_);
                if (!torrent.running_)
                    break;
            }

            torrent.ProcessPeers();

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    void Torrent::Start()
    {
        std::lock_guard lock(mutex_);

        if (running_)
            return;

        running_ = true;
        thread_ = std::thread(Torrent::Run, std::ref(*this));

        LOG("Torrent", "Torrent started");
    }

    void Torrent::Stop()
    {
        std::lock_guard lock(mutex_);

        running_ = false;
        if (thread_.joinable())
            thread_.join();

        LOG("Torrent", "Torrent stopped");
    }

    std::shared_ptr<const Metainfo> Torrent::GetMetainfo() const
    {
        return metainfo_;
    }

    const PeerId &Torrent::GetPeerId() const
    {
        return peer_id_;
    }

    void Torrent::AddPeer(const PeerInfo &peer_info)
    {
        if (std::find(potential_peers_.begin(), potential_peers_.end(), peer_info) != potential_peers_.end())
            return;
        if (std::find_if(current_peers_.begin(), current_peers_.end(), [&peer_info](const auto &peer)
                         { return peer.GetPeerInfo() == peer_info; }) != current_peers_.end())
            return;

        potential_peers_.push_back(peer_info);
    }

    void Torrent::ProcessPeers()
    {
        // Add new peers if we don't have enough and there are still peers in the queue
        // TODO send a tracker request if we need more peers
        while (current_peers_.size() < DESIRED_PEERS && !potential_peers_.empty())
        {
            PeerInfo peer_info = potential_peers_.front();
            potential_peers_.pop_front();

            LOG("Torrent", "Adding peer %s %u", peer_info.ip.c_str(), peer_info.port);
            current_peers_.emplace_back(piece_manager_, peer_info, metainfo_, peer_id_);

            if (event_queue_.EventEnabled(EventType::PeerStatusChanged))
                event_queue_.PushEvent(std::make_unique<PeerStatusChangedEvent>(TorrentHandle(shared_from_this()), peer_info.ip, peer_info.port, PeerStatus::Connecting));
        }

        auto it = current_peers_.begin();
        while (it != current_peers_.end())
        {
            if (!it->Process())
            {
                const auto &info = it->GetPeerInfo();
                (void)info;
                LOG("Torrent", "Peer %s %u finished", info.ip.c_str(), info.port);

                if (event_queue_.EventEnabled(EventType::PeerStatusChanged))
                    event_queue_.PushEvent(std::make_unique<PeerStatusChangedEvent>(TorrentHandle(shared_from_this()), info.ip, info.port, PeerStatus::Disconnected));

                it = current_peers_.erase(it);
                continue;
            }

            ++it;
        }
    }
}