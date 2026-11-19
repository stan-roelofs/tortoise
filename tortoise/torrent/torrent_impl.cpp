#include "torrent_impl.hpp"

#include <cassert>

#include "../util/log.hpp"

namespace {
constexpr std::chrono::seconds PEER_REQUEST_INTERVAL = std::chrono::seconds(20);
}

namespace tortoise {
bool TorrentHandle::IsValid() const { return !ptr_.expired(); }

std::shared_ptr<const Metainfo> TorrentHandle::GetMetainfo() const {
  if (!IsValid())
    throw InvalidHandleException("TorrentHandle is not valid");

  return ptr_.lock()->GetMetainfo();
}

bool TorrentHandle::StartDownload() {
  if (!IsValid())
    throw InvalidHandleException("TorrentHandle is not valid");

  return ptr_.lock()->RequestStart();
}

void TorrentHandle::StopDownload() {
  if (!IsValid())
    throw InvalidHandleException("TorrentHandle is not valid");

  ptr_.lock()->RequestStop(false);
}

Statistics TorrentHandle::GetStatistics() const {
  if (!IsValid())
    throw InvalidHandleException("TorrentHandle is not valid");

  return ptr_.lock()->GetStatistics();
}

TorrentState TorrentHandle::GetState() const {
  if (!IsValid())
    throw InvalidHandleException("TorrentHandle is not valid");

  return ptr_.lock()->GetState();
}

TorrentHandle::operator bool() const { return IsValid(); }

bool TorrentHandle::operator==(const TorrentHandle &other) const { return ptr_.lock() == other.ptr_.lock(); }

bool TorrentHandle::operator!=(const TorrentHandle &other) const { return !(*this == other); }

Torrent::Torrent(TorrentParameters parameters, Torrent::PeerInfoProvider &peer_info_provider, EventQueue &event_queue)
    : parameters_(std::make_shared<TorrentParameters>(parameters)), metainfo_(std::make_shared<Metainfo>(parameters.metainfo)),
      event_queue_(event_queue), piece_manager_(metainfo_, parameters.save_path), peer_info_provider_(peer_info_provider),
      state_(TorrentState::Stopped) {
  peer_info_provider_.RegisterTorrent(*this, std::bind(&Torrent::OnNewPeers, this, std::placeholders::_1));
  piece_manager_.AddListener(this);
}

Torrent::~Torrent() {
  RequestStop(true);

  piece_manager_.RemoveListener(this);

  std::scoped_lock lock(peer_mutex_);
  peer_info_provider_.UnregisterTorrent(*this);
}

void Torrent::Run(std::stop_token token) {
  LOG_INFO("Torrent", "Torrent started");

  while (!token.stop_requested()) {
    switch (GetState()) {
    case TorrentState::Preparing:
      if (piece_manager_.FilesPrepared()) {
        LOG_INFO("Torrent", "Torrent files prepared");
        SetState(TorrentState::Downloading);
      }
      break;

    case TorrentState::Downloading:
    case TorrentState::Seeding:
      ProcessPeers();
      UpdateStatistics();
      break;

    case TorrentState::Stopped:
      break;
    }
  }

  LOG_INFO("Torrent", "Torrent stopped");
  SetState(TorrentState::Stopped);
}

void Torrent::SetState(TorrentState status) {
  std::scoped_lock lock(data_mutex_);
  if (state_ == status)
    return;
  state_ = status;
  event_queue_.Push(event::TorrentStatusChanged{weak_from_this(), status});
}

bool Torrent::RequestStart() {
  std::scoped_lock lock(data_mutex_);

  if (state_ != TorrentState::Stopped)
    return false;
  SetState(TorrentState::Preparing);

  thread_ = std::jthread([this](std::stop_token token) { Run(token); });
  return true;
}

void Torrent::RequestStop(bool wait) {
  {
    std::scoped_lock lock(data_mutex_);

    if (state_ == TorrentState::Stopped)
      return;

    thread_.request_stop();
  }
  if (wait && thread_.joinable())
    thread_.join();
}

std::shared_ptr<const Metainfo> Torrent::GetMetainfo() const { return metainfo_; }

PeerId Torrent::GetPeerId() const { return peer_id_; }

Statistics Torrent::GetStatistics() const {
  std::scoped_lock lock(data_mutex_);
  return statistics_;
}

TorrentState Torrent::GetState() const {
  std::scoped_lock lock(data_mutex_);
  return state_;
}

void Torrent::OnNewPeers(const std::vector<PeerInfo> &new_peers) {
  std::scoped_lock lock(peer_mutex_);

  for (const auto &peer_info : new_peers) {
    if (std::find(potential_peers_.begin(), potential_peers_.end(), peer_info) != potential_peers_.end())
      return;
    if (std::find_if(peers_.begin(), peers_.end(), [&peer_info](const auto &peer) { return peer.first->GetPeerInfo() == peer_info; }) !=
        peers_.end())
      return;
    potential_peers_.push_back(peer_info);
  }
}

void Torrent::OnPieceDownloaded(std::uint32_t piece_index) { event_queue_.Push(event::PieceDownloaded{weak_from_this(), piece_index}); }

void Torrent::OnWriteError() {
  event_queue_.Push(event::TorrentError{weak_from_this(), event::TorrentError::Code::FileError});
  RequestStop(false);
}

void Torrent::OnTorrentDownloaded() {
  state_ = TorrentState::Seeding;
  event_queue_.Push(event::TorrentStatusChanged{weak_from_this(), TorrentState::Seeding});
}

void Torrent::RequestPeers() {
  LOG_INFO("Torrent", "Requesting peers");
  last_peer_request_ = std::chrono::steady_clock::now();
  peer_info_provider_.RequestPeers(*this, parameters_->max_peers * 2);
}

void Torrent::UpdateStatistics() {
  // Update rates...
  std::uint64_t download_speed = 0;
  std::uint64_t upload_speed = 0;
  for (const auto &peer : peers_) {
    download_speed += peer.first->GetDownloadSpeed();
    upload_speed += peer.first->GetUploadSpeed();
  }

  std::scoped_lock lock(data_mutex_);
  std::swap(statistics_.download_rate, download_speed);
  std::swap(statistics_.upload_rate, upload_speed);
}

void Torrent::ProcessPeers() {
  const auto peers_needed = parameters_->max_peers - (potential_peers_.size() + peers_.size());

  const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
  if (peers_needed && (last_peer_request_.time_since_epoch() == std::chrono::steady_clock::duration(0) ||
                       (now - last_peer_request_) > PEER_REQUEST_INTERVAL))
    RequestPeers();

  std::scoped_lock lock(peer_mutex_);

  // Add new peers if we don't have enough and there are still peers in the queue
  // TODO send a tracker request if we need more peers
  while (peers_.size() < parameters_->max_peers && !potential_peers_.empty()) {
    PeerInfo peer_info = potential_peers_.front();
    potential_peers_.pop_front();

    LOG_INFO("Torrent", std::format("Pending peer {}", peer_info.ToString()));
    peers_.push_back({std::make_unique<Peer>(peer_info, parameters_, metainfo_, peer_id_, piece_manager_), PeerStatus::Connecting});
    event_queue_.Push(event::PeerStatusChanged{weak_from_this(), peer_info, PeerStatus::Connecting});
  }

  {
    auto it = peers_.begin();
    while (it != peers_.end()) {
      const auto status = it->first->Process();
      if (status != it->second) {
        it->second = status;
        event_queue_.Push(event::PeerStatusChanged{weak_from_this(), it->first->GetPeerInfo(), status});
      }

      // TODO: if peer slow or has no pieces we need AND there are potential peers, disconnect and connect to a new peer

      switch (status) {
      case PeerStatus::Connecting:
      case PeerStatus::Connected:
        ++it;
        break;
      case PeerStatus::Disconnected: {
        it = peers_.erase(it);
        break;
      }
      }
    }
  }
}
} // namespace tortoise