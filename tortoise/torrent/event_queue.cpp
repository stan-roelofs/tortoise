#include "event_queue.hpp"

#include <assert.h>

namespace tortoise
{
    Event::Event(EventType type) : type(type)
    {
    }

    Event::~Event() = default;

    EventQueue::EventQueue(const EventCallbacks &callbacks) : callbacks_(callbacks)
    {
    }

    EventQueue::~EventQueue() = default;

    bool EventQueue::EventEnabled(EventType type) const
    {
        switch (type)
        {
        case EventType::TorrentAdded:
            return callbacks_.torrent_added != nullptr;
        case EventType::PeerStatusChanged:
            return callbacks_.peer_status_changed != nullptr;
        }

        assert(false);
        return false;
    }

    void EventQueue::PushEvent(std::unique_ptr<Event> event)
    {
        std::lock_guard lock(mutex_);
        events_.push_back(std::move(event));

        // TODO if size > something, throw away old events
    }

    std::vector<std::unique_ptr<Event>> EventQueue::PopEvents()
    {
        std::lock_guard lock(mutex_);
        std::vector<std::unique_ptr<Event>> events;
        events.swap(events_);
        return events;
    }
}