#include "app.hpp"

#include <assert.h>
#include <csignal>
#include <format>
#include <fstream>
#include <iostream>

#include <string>
#include <tortoise/load_torrent.hpp>
#include <tortoise/logging.hpp>
#include <tortoise/metainfo.hpp>

namespace {
std::atomic_bool running_;
std::ofstream log_stream_;
tortoise::logging::Level log_level_ = tortoise::logging::Level::Info;

void Log(const tortoise::logging::Message &message) {
  if (!log_stream_.is_open() || message.level < log_level_)
    return;

  log_stream_ << std::format("{:L%T} ", message.time);

  switch (message.level) {
  case tortoise::logging::Level::Debug:
    log_stream_ << "[DEBUG] ";
    break;
  case tortoise::logging::Level::Info:
    log_stream_ << "[INFO] ";
    break;
  case tortoise::logging::Level::Warning:
    log_stream_ << "[WARNING] ";
    break;
  case tortoise::logging::Level::Error:
    log_stream_ << "[ERROR] ";
    break;
  }

  log_stream_ << std::format("{} : {}", message.tag, message.message) << std::endl;
}

unsigned GetConsoleWidth() {
  return 160; // TODO: Implement a proper way to get the console width on different platforms.
}

void HideConsoleCursor() { std::cout << "\033[?25l" << std::flush; }

std::string GetDownloadRate(const tortoise::TorrentHandle &handle) {
  const char *units[] = {"B", "KB", "MB", "GB", "TB"};
  int unit_index = 0;

  double display_size = static_cast<double>(handle.GetStatistics().download_rate);
  while (display_size >= 1024 && unit_index < 4) {
    display_size /= 1024;
    ++unit_index;
  }

  return std::format("{:.2f} {}/s", display_size, units[unit_index]);
}

std::string GetProgressBar(unsigned width, unsigned pieces_downloaded, unsigned total_pieces) {
  if (total_pieces == 0)
    return std::string(width, ' ');

  const auto expected_width = width;

  const std::string pieces_str = std::format("{}/{}", pieces_downloaded, total_pieces);
  width -= pieces_str.size();

  const double progress = static_cast<double>(pieces_downloaded) / total_pieces;
  const std::string progress_str = std::format("{:.2f}%", progress * 100);
  width -= progress_str.size();

  width -= 6; // for the brackets and spaces
  const unsigned progress_width = static_cast<unsigned>(width * progress);

  std::string result = "[";
  result += std::string(progress_width, '=');
  result += std::string(width - progress_width, ' ');
  result += "]";
  result += std::format(" {} | {}", progress_str, pieces_str);

  assert(result.size() == expected_width);
  return result;
}
} // namespace

Application::Application(CommandLineArguments args) : args_(args), handle_({}) {
  log_stream_ = std::ofstream("tortoise.log");
  tortoise::logging::RegisterReceiver(Log);

  tortoise::event::Callbacks event_callbacks;
  event_callbacks.torrent_status_changed = std::bind(&Application::OnTorrentStatusChanged, this, std::placeholders::_1);
  event_callbacks.torrent_error = std::bind(&Application::OnTorrentError, this, std::placeholders::_1);
  event_callbacks.peer_status_changed = std::bind(&Application::OnPeerStatusChanged, this, std::placeholders::_1);
  event_callbacks.piece_downloaded = std::bind(&Application::OnPieceDownloaded, this, std::placeholders::_1);

  session_ = std::make_unique<tortoise::Session>(event_callbacks);
}

bool Application::AddTorrent(const std::string &torrent_file) {
  auto metainfo = tortoise::LoadTorrent(torrent_file);
  if (!metainfo) {
    std::cout << "Failed to load torrent file: " << torrent_file << std::endl;
    return false;
  }

  tortoise::TorrentParameters torrent_params(*metainfo);
  torrent_params.save_path = ".";
  auto handle = session_->AddTorrent(torrent_params);
  if (!handle) {
    std::cout << "Failed to add torrent" << std::endl;
    return false;
  }

  data_ = std::make_unique<TorrentInfo>(handle.GetMetainfo()->pieces.size());

  handle.StartDownload();
  handle_ = handle;
  return true;
}

void Application::OnTorrentStatusChanged(const tortoise::event::TorrentStatusChanged &event) {
  switch (event.status) {
  case tortoise::TorrentStatus::Downloading:
    // std::cout << "Torrent downloading: " << event.handle.GetMetainfo()->name << std::endl;
    break;
  case tortoise::TorrentStatus::Seeding:
    std::cout << "Torrent downloaded: " << event.handle.GetMetainfo()->name << std::endl;
    running_ = false;
    break;
  case tortoise::TorrentStatus::Stopped:
    // std::cout << "Torrent stopped: " << event.handle.GetMetainfo()->name << std::endl;
    break;
  }
}

void Application::OnTorrentError(const tortoise::event::TorrentError &event) {
  std::cout << "Torrent error: " << event.handle.GetMetainfo()->name << std::endl;
  running_ = false;
}

void Application::OnPeerStatusChanged(const tortoise::event::PeerStatusChanged &event) {
  data_->peers[event.info] = event.status;
  switch (event.status) {
  case tortoise::PeerStatus::Connecting:
    // std::cout << std::format("Connecting to peer {}", event.info.ToString()) << std::endl;
    break;
  case tortoise::PeerStatus::Connected:
    // std::cout << std::format("Connected to peer {}", event.info.ToString()) << std::endl;
    break;
  case tortoise::PeerStatus::Disconnected:
    // std::cout << std::format("Disconnected from peer {}", event.info.ToString()) << std::endl;
    data_->peers.erase(event.info);
    break;
  }
}

void Application::OnPieceDownloaded(const tortoise::event::PieceDownloaded &event) {
  ++data_->pieces_downloaded;
  assert(!data_->have_pieces.at(event.piece_index));
  data_->have_pieces.at(event.piece_index) = true;
}

int Application::Run() {
  assert(!running_);
  if (running_)
    return EXIT_FAILURE;

  if (!AddTorrent(args_.torrent_file))
    return EXIT_FAILURE;

  const auto SignalHandler = [](int signal) {
    if (running_)
      std::cout << std::format("Shutting down...", signal) << std::endl;
    running_ = false;
  };

  std::signal(SIGINT, SignalHandler);
  std::signal(SIGTERM, SignalHandler);
  std::signal(SIGABRT, SignalHandler);
  std::signal(SIGPIPE, SIG_IGN);

  running_ = true;
  HideConsoleCursor();
  const auto name = handle_.GetMetainfo()->name.substr(0, 20);
  while (running_) {
    session_->HandleEvents();

    if (handle_.GetStatus() == tortoise::TorrentStatus::Downloading) {
      const std::string peers = std::format("Peers: {}", data_->peers.size());
      const std::string download_rate = GetDownloadRate(handle_);
      auto progress_bar_width = GetConsoleWidth() - name.size() - download_rate.size() - peers.size() - 6;
      std::cout << std::format("{} {} | {} | {}\r", name,
                               GetProgressBar(progress_bar_width, data_->pieces_downloaded, data_->have_pieces.size()), download_rate,
                               peers)
                << std::flush;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return EXIT_SUCCESS;
}