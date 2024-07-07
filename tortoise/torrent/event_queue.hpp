#ifndef TORTOISE_EVENT_QUEUE_HPP
#define TORTOISE_EVENT_QUEUE_HPP

#include <tortoise/event.hpp>

#include <memory>
#include <mutex>
#include <vector>

namespace tortoise
{
    enum class EventType
    {
        TorrentAdded,
        PeerStatusChanged,
    };

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
        TorrentAddedEvent() : Event(EventType::TorrentAdded) {}
    };

    struct PeerStatusChangedEvent : Event
    {
        PeerStatusChangedEvent(const std::string &ip, std::uint16_t port, PeerStatus status) : Event(EventType::PeerStatusChanged), ip(ip), port(port), status(status) {}
        const std::string ip;
        const std::uint16_t port;
        PeerStatus status;
    };

    class EventQueue
    {
    public:
        EventQueue(const EventCallbacks &callbacks);
        virtual ~EventQueue();

        bool EventEnabled(EventType type) const;
        void PushEvent(std::unique_ptr<Event> event);

        std::vector<std::unique_ptr<Event>> PopEvents();

    private:
        std::mutex mutex_;
        std::vector<std::unique_ptr<Event>> events_;
        const EventCallbacks callbacks_;
    };
}

#endif