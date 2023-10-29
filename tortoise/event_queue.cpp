#include "event_queue.hpp"

namespace tortoise
{
    Event::Event(EventType type) : type(type)
    {
    }

    Event::~Event() = default;

    EventQueue::EventQueue(event_bitset enabled_events) : enabled_events_(enabled_events)
    {
    }

    EventQueue::~EventQueue()
    {
    }

    bool EventQueue::EventEnabled(EventType type) const
    {
        return enabled_events_.test(static_cast<std::size_t>(type)) == 1;
    }

    void EventQueue::PushEvent(std::unique_ptr<Event> event)
    {
        std::lock_guard lock(mutex_);
        events_.push_back(std::move(event));
    }

    std::vector<std::unique_ptr<Event>> EventQueue::PopEvents()
    {
        std::lock_guard lock(mutex_);
        std::vector<std::unique_ptr<Event>> events;
        events.swap(events_);
        return events;
    }
}