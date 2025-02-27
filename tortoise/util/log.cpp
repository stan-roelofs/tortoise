#include "log.hpp"

#include <cinttypes>
#include <condition_variable>
#include <queue>
#include <mutex>
#include <thread>

namespace tortoise
{
	namespace logging
	{
		class Log
		{
		public:
			Log() = default;
			~Log()
			{
				if (thread_)
					thread_->request_stop();
				cv_.notify_all();
			}

			Log(const Log&) = delete;
			Log& operator=(const Log&) = delete;
			Log(Log&&) = delete;
			Log& operator=(Log&&) = delete;

			void SetReceiver(LogReceiver receiver)
			{
				std::scoped_lock lock(mutex_);
				if (receiver && !thread_)
					thread_ = std::make_unique<std::jthread>(Log::ProcessMessages, this);
				receiver_ = receiver;
			}

			void LogMessage(Level level, std::string_view tag, std::string message)
			{
				std::scoped_lock guard(mutex_);

				if (!receiver_)
					return;
				messages_.emplace(std::chrono::system_clock::now(), level, std::string(tag), std::move(message));
				cv_.notify_one();
			}

		private:
			static void ProcessMessages(std::stop_token stop_token, Log* log)
			{
				while (!stop_token.stop_requested())
				{
					std::unique_lock<std::mutex> lock(log->mutex_);
					log->cv_.wait(lock);

					while (!log->messages_.empty())
					{
						auto message = log->messages_.front();
						log->messages_.pop();
						lock.unlock();

						log->receiver_(message);

						lock.lock();
					}
				}
			}

			LogReceiver receiver_;
			std::unique_ptr<std::jthread> thread_;
			std::mutex mutex_;
			std::condition_variable cv_;

			std::queue<Message> messages_;
		};

		static Log log_;
		void RegisterReceiver(LogReceiver receiver)
		{
			log_.SetReceiver(receiver);
		}

		void LogMessage(Level level, std::string_view tag, std::string message)
		{
			log_.LogMessage(level, tag, message);
		}
	} // namespace logging
} // namespace tortoise