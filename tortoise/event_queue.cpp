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

    EventQueue::~EventQueue()
    {
    }

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
    }

    void EventQueue::HandleEvents()
    {
        mutex_.lock();
        std::vector<std::unique_ptr<Event>> events;
        events.swap(events_);
        mutex_.unlock();

        for (auto &event : events)
        {
            Event *e = event.get();

            // Note: we could use a visitor pattern here, but it is a lot of effort with little benefit.
            if (auto *added_event = dynamic_cast<TorrentAddedEvent *>(e))
            {
                assert(callbacks_.torrent_added);
                callbacks_.torrent_added(added_event->torrent);
            }
            else if (auto *peer_status_event = dynamic_cast<PeerStatusChangedEvent *>(e))
            {
                assert(callbacks_.peer_status_changed);
                callbacks_.peer_status_changed(peer_status_event->torrent, peer_status_event->ip, peer_status_event->port, peer_status_event->status);
            }
        }
    }
}