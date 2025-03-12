#include "app.hpp"

#include <assert.h>
#include <csignal>
#include <format>
#include <iostream>
#include <fstream>

#include <tortoise/load_torrent.hpp>
#include <tortoise/logging.hpp>
#include <tortoise/metainfo.hpp>

namespace
{
	std::atomic_bool running_;

	void Log(const tortoise::logging::Message &message)
	{
		static auto stream = std::ofstream("tortoise.log", std::ios::app);
		assert(stream.is_open());

		stream << std::format("{:L%T} ", message.time);

		switch (message.level)
		{
		case tortoise::logging::Level::Debug:
			stream << "[DEBUG] ";
			break;
		case tortoise::logging::Level::Info:
			stream << "[INFO] ";
			break;
		case tortoise::logging::Level::Warning:
			stream << "[WARNING] ";
			break;
		case tortoise::logging::Level::Error:
			stream << "[ERROR] ";
			break;
		}

		stream << std::format("{} : {}", message.tag, message.message) << std::endl;
	}
}

Application::Application(CommandLineArguments args) : args_(args)
{
	tortoise::logging::RegisterReceiver(Log);

	tortoise::event::Callbacks event_callbacks;
	event_callbacks.torrent_added = std::bind(&Application::OnTorrentAdded, this, std::placeholders::_1);
	event_callbacks.torrent_started = std::bind(&Application::OnTorrentStarted, this, std::placeholders::_1);
	event_callbacks.torrent_stopped = std::bind(&Application::OnTorrentStopped, this, std::placeholders::_1);
	event_callbacks.torrent_downloaded = std::bind(&Application::OnTorrentDownloaded, this, std::placeholders::_1);
	event_callbacks.torrent_error = std::bind(&Application::OnTorrentError, this, std::placeholders::_1);
	event_callbacks.peer_status_changed = std::bind(&Application::OnPeerStatusChanged, this, std::placeholders::_1);
	event_callbacks.piece_downloaded = std::bind(&Application::OnPieceDownloaded, this, std::placeholders::_1);

	session_ = std::make_unique<tortoise::Session>(event_callbacks);
}

bool Application::AddTorrent(const std::string &torrent_file)
{
	auto metainfo = tortoise::LoadTorrent(torrent_file);
	if (!metainfo)
	{
		std::cout << "Failed to load torrent file: " << torrent_file << std::endl;
		return false;
	}

	tortoise::TorrentParameters torrent_params(*metainfo);
	torrent_params.save_path = ".";
	auto handle = session_->AddTorrent(torrent_params);
	if (!handle)
	{
		std::cout << "Failed to add torrent" << std::endl;
		return false;
	}

	handle.StartDownload();

	torrents_.emplace(handle, handle);

	return true;
}

void Application::OnTorrentAdded(const tortoise::event::TorrentAdded &event)
{
	std::cout << "Torrent added: " << event.handle.GetMetainfo().name << std::endl;
}

void Application::OnTorrentStarted(const tortoise::event::TorrentStarted &event)
{
	std::cout << "Torrent started: " << event.handle.GetMetainfo().name << std::endl;
}

void Application::OnTorrentStopped(const tortoise::event::TorrentStopped &event)
{
	std::cout << "Torrent stopped: " << event.handle.GetMetainfo().name << std::endl;
}

void Application::OnTorrentDownloaded(const tortoise::event::TorrentDownloaded &event)
{
	std::cout << "Torrent downloaded: " << event.handle.GetMetainfo().name << std::endl;
	session_->RemoveTorrent(event.handle);
}

void Application::OnTorrentError(const tortoise::event::TorrentError &event)
{
	std::cout << "Torrent error: " << event.handle.GetMetainfo().name << std::endl;
}

void Application::OnPeerStatusChanged(const tortoise::event::PeerStatusChanged &event)
{
	auto it = torrents_.find(event.handle);
	if (it == torrents_.end())
		throw tortoise::Exception("Invalid handle");

	it->second.peers[event.info] = event.status;
	switch (event.status)
	{
	case tortoise::PeerStatus::Connecting:
		std::cout << std::format("[{}] : Connecting to peer {}", event.handle.GetMetainfo().name, event.info.ToString()) << std::endl;

		break;
	case tortoise::PeerStatus::Connected:
		std::cout << std::format("[{}] : Connected to peer {}", event.handle.GetMetainfo().name, event.info.ToString()) << std::endl;
		break;
	case tortoise::PeerStatus::Disconnected:
		std::cout << std::format("[{}] : Disconnected from peer {}", event.handle.GetMetainfo().name, event.info.ToString()) << std::endl;
		it->second.peers.erase(event.info);
		break;
	}
}

void Application::OnPieceDownloaded(const tortoise::event::PieceDownloaded &event)
{
	auto it = torrents_.find(event.handle);
	if (it == torrents_.end())
		throw tortoise::Exception("Invalid handle");

	++it->second.pieces_downloaded;
	assert(!it->second.have_pieces.at(event.piece_index));
	it->second.have_pieces.at(event.piece_index) = true;

	const auto download_rate_mb = event.handle.GetStatistics().download_rate / 1024.0f / 1024.0f;

	std::cout << std::format("[{}] : Downloaded {}/{} pieces from {} peers ({:.2f} MB/s)", event.handle.GetMetainfo().name, it->second.pieces_downloaded, event.handle.GetMetainfo().pieces.size(), it->second.peers.size(), download_rate_mb) << std::endl;
}

int Application::Run()
{
	assert(!running_);
	if (running_)
		return EXIT_FAILURE;

	if (!AddTorrent(args_.torrent_file))
		return EXIT_FAILURE;

	const auto SignalHandler = [](int signal)
	{
		if (running_)
			std::cout << std::format("Shutting down...", signal) << std::endl;
		running_ = false;
	};

	std::signal(SIGINT, SignalHandler);
	std::signal(SIGTERM, SignalHandler);
	std::signal(SIGABRT, SignalHandler);

	running_ = true;
	while (running_)
	{
		session_->HandleEvents();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return EXIT_SUCCESS;
}