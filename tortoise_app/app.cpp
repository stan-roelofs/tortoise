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
	std::ofstream log_stream;

	void Log(const tortoise::logging::Message &message)
	{
		if (!log_stream.is_open())
			return;

		log_stream << std::format("{:L%T} ", message.time);

		switch (message.level)
		{
		case tortoise::logging::Level::Debug:
			log_stream << "[DEBUG] ";
			break;
		case tortoise::logging::Level::Info:
			log_stream << "[INFO] ";
			break;
		case tortoise::logging::Level::Warning:
			log_stream << "[WARNING] ";
			break;
		case tortoise::logging::Level::Error:
			log_stream << "[ERROR] ";
			break;
		}

		log_stream << std::format("{} : {}", message.tag, message.message) << std::endl;
	}
}

Application::Application(CommandLineArguments args) : args_(args), handle_({})
{
	log_stream = std::ofstream("tortoise.log");
	tortoise::logging::RegisterReceiver(Log);

	tortoise::event::Callbacks event_callbacks;
	event_callbacks.torrent_status_changed = std::bind(&Application::OnTorrentStatusChanged, this, std::placeholders::_1);
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

	data_ = std::make_unique<TorrentInfo>(handle.GetMetainfo()->pieces.size());

	handle.StartDownload();
	handle_ = handle;
	return true;
}

void Application::OnTorrentStatusChanged(const tortoise::event::TorrentStatusChanged &event)
{
	switch (event.status)
	{
	case tortoise::TorrentStatus::Downloading:
		std::cout << "Torrent downloading: " << event.handle.GetMetainfo()->name << std::endl;
		break;
	case tortoise::TorrentStatus::Seeding:
		std::cout << "Torrent downloaded: " << event.handle.GetMetainfo()->name << std::endl;
		running_ = false;
		break;
	case tortoise::TorrentStatus::Stopped:
		std::cout << "Torrent stopped: " << event.handle.GetMetainfo()->name << std::endl;
		break;
	}
}

void Application::OnTorrentError(const tortoise::event::TorrentError &event)
{
	std::cout << "Torrent error: " << event.handle.GetMetainfo()->name << std::endl;
}

void Application::OnPeerStatusChanged(const tortoise::event::PeerStatusChanged &event)
{
	data_->peers[event.info] = event.status;
	switch (event.status)
	{
	case tortoise::PeerStatus::Connecting:
		std::cout << std::format("Connecting to peer {}", event.info.ToString()) << std::endl;
		break;
	case tortoise::PeerStatus::Connected:
		std::cout << std::format("Connected to peer {}", event.info.ToString()) << std::endl;
		break;
	case tortoise::PeerStatus::Disconnected:
		std::cout << std::format("Disconnected from peer {}", event.info.ToString()) << std::endl;
		data_->peers.erase(event.info);
		break;
	}
}

void Application::OnPieceDownloaded(const tortoise::event::PieceDownloaded &event)
{
	++data_->pieces_downloaded;
	assert(!data_->have_pieces.at(event.piece_index));
	data_->have_pieces.at(event.piece_index) = true;
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

		if (handle_.GetStatus() == tortoise::TorrentStatus::Downloading)
		{
			const auto download_rate_mb = handle_.GetStatistics().download_rate / 1024.0f / 1024.0f;
			std::cout << std::format("Downloaded {}/{} pieces from {} peers ({:.2f} MB/s)", data_->pieces_downloaded, data_->have_pieces.size(), data_->peers.size(), download_rate_mb) << std::endl;
		}

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	return EXIT_SUCCESS;
}