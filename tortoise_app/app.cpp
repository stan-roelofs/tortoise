#include "app.hpp"

#include <assert.h>
#include <format>
#include <iostream>
#include <fstream>

#include <tortoise/load_torrent.hpp>
#include <tortoise/logging.hpp>
#include <tortoise/metainfo.hpp>

namespace
{
	void Log(const tortoise::logging::Message& message)
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
	event_callbacks.peer_status_changed = std::bind(&Application::OnPeerStatusChanged, this, std::placeholders::_1);

	session_ = std::make_unique<tortoise::Session>(event_callbacks);
}

bool Application::AddTorrent(const std::string& torrent_file)
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

	torrents_.emplace_back(handle);

	return true;
}

void Application::OnTorrentAdded(const tortoise::event::TorrentAdded& event)
{
	std::cout << "Torrent added: " << event.handle.GetMetainfo().name << std::endl;
}

void Application::OnPeerStatusChanged(const tortoise::event::PeerStatusChanged& event)
{
	switch (event.status)
	{
	case tortoise::PeerStatus::Connecting:
		std::cout << "Connecting to peer " << event.info.ToString() << std::endl;
		break;
	case tortoise::PeerStatus::Connected:
		std::cout << "Connected to peer " << event.info.ToString() << std::endl;
		break;
	case tortoise::PeerStatus::Disconnected:
		std::cout << "Disconnected from peer " << event.info.ToString() << std::endl;
		break;
	case tortoise::PeerStatus::Unknown:
		break;
	}
}

int Application::Run()
{
	if (!AddTorrent(args_.torrent_file))
		return EXIT_FAILURE;

	running_ = true;
	while (running_)
	{
		session_->HandleEvents();

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return EXIT_SUCCESS;
}