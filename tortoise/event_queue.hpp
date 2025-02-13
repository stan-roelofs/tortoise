#ifndef TORTOISE_EVENT_QUEUE_HPP
#define TORTOISE_EVENT_QUEUE_HPP

#include <cassert>
#include <cinttypes>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <queue>
#include <typeindex>

#include <tortoise/exception.hpp>

namespace tortoise
{
	/*! \brief A thread-safe queue that can accept events of any type and notify subscribers of the event.*/
	class EventQueue
	{
	public:
		using Token = std::uint64_t;

		EventQueue() : next_token_(0) {}

		/*! \brief Subscribe to an event type.
		 *
		 * \param callback The callback to be called when the event is triggered.
		 * \return A token that identifies the subscription.
		 */
		template <class T>
		Token Subscribe(const std::function<void(const T &)> &callback)
		{
			std::type_index type = std::type_index(typeid(T));

			std::lock_guard lock(subscribers_.mutex);

			Token token;
			if (!subscribers_.removed.empty())
			{
				token = subscribers_.removed.front();
				subscribers_.removed.pop();
			}
			else
				token = next_token_++;

			const auto [it, success] = subscribers_.current[type].insert({token, [callback](const void *event)
																		  { callback(*static_cast<const T *>(event)); }});
			if (!success)
				throw Exception("Failed to subscribe to event");
			return token;
		}

		template <class T>
		void Push(const T &event)
		{
			std::lock_guard lock(queued_events_.mutex);
			queued_events_.values.push_back([this, event]
											{ ProcessEvent(event); });
		}

		//! \brief Processes all events in the queue, this should be called regularly.
		void Process()
		{
			for (;;)
			{
				std::function<void()> event;
				{
					std::lock_guard lock(queued_events_.mutex);
					if (queued_events_.values.empty())
						break;

					event = queued_events_.values.front();
					queued_events_.values.pop_front();
				}

				event();
			}
		}

	private:
		void Unsubscribe(std::type_index type, Token token)
		{
			std::lock_guard lock(subscribers_.mutex);
			subscribers_.current[type].erase(token);
			subscribers_.removed.push(token);
		}

		template <class T>
		void ProcessEvent(const T &event)
		{
			auto type = std::type_index(typeid(T));

			std::lock_guard lock(subscribers_.mutex);
			for (auto &subscriber : subscribers_.current[type])
				subscriber.second(&event);
		}

		struct
		{
			std::map<std::type_index, std::map<Token, std::function<void(const void *)>>> current;
			std::queue<Token> removed;
			std::mutex mutex;
		} subscribers_;

		struct
		{
			std::list<std::function<void()>> values;
			std::mutex mutex;
		} queued_events_;

		Token next_token_;
	};
} // namespace tortoise

#endif