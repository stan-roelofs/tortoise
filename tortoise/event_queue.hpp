#ifndef TORTOISE_EVENT_QUEUE_HPP
#define TORTOISE_EVENT_QUEUE_HPP

#include <tortoise/event.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace tortoise
{
    class Event
    {
    public:
        const EventType type;
        virtual ~Event();

    protected:
        Event(EventType type);
    };

    struct TorrentAddedEvent : Event
    {
        TorrentAddedEvent(TorrentHandle torrent) : Event(EventType::TorrentAdded), torrent(torrent) {}
        TorrentHandle torrent;
    };

    struct PeerStatusChangedEvent : Event
    {
        PeerStatusChangedEvent(TorrentHandle torrent, const std::string &ip, std::uint16_t port, PeerStatus status) : Event(EventType::PeerStatusChanged), torrent(torrent), ip(ip), port(port), status(status) {}
        TorrentHandle torrent;
        const std::string ip;
        const std::uint16_t port;
        PeerStatus status;
    };

    class EventQueue
    {
    public:
        EventQueue(event_bitset enabled_events);
        ~EventQueue();

        bool EventEnabled(EventType type) const;
        void PushEvent(std::unique_ptr<Event> event);
        std::vector<std::unique_ptr<Event>> PopEvents();

    private:
        std::mutex mutex_;
        std::vector<std::unique_ptr<Event>> events_;
        event_bitset enabled_events_;
    };
}

#endif