#ifndef TORTOISE_TRACKER_CONNECTION_HPP
#define TORTOISE_TRACKER_CONNECTION_HPP

#include <atomic>
#include <chrono>
#include <memory>

#include "../network/socket.hpp"
#include "../util//util.hpp"
#include "announce.hpp"

namespace httplib {
class Client;
} // namespace httplib

namespace tortoise {
namespace tracker {

class Connection {
public:
  static std::unique_ptr<Connection> Create(const network::URL &url);

  virtual ~Connection() = default;
  virtual std::optional<AnnounceResponse> Announce(std::shared_ptr<const AnnounceParameters> parameters) = 0;
  virtual void Cancel() = 0;
};

class HTTPConnection : public Connection {
public:
  HTTPConnection(const network::URL &url);
  ~HTTPConnection() override;
  std::optional<AnnounceResponse> Announce(std::shared_ptr<const AnnounceParameters> parameters) override;
  void Cancel() override;

private:
  network::URL url_;
  std::unique_ptr<httplib::Client> client_;
};

class UDPConnection : public Connection {
public:
  UDPConnection(const network::URL &url);
  ~UDPConnection();
  void Cancel() override;

  std::optional<AnnounceResponse> Announce(std::shared_ptr<const AnnounceParameters> parameters) override;

private:
  enum class SendResult { Finished, Unfinished, Error };

  enum class ReceiveResult { Finished, Unfinished, Timeout, Error };

  void CreateConnectRequest();
  void CreateAnnounceRequest();
  SendResult Send();
  ReceiveResult Receive();
  void ResizeBuffer(std::size_t size);

  enum class State { SendConnectRequest, ReceiveConnectResponse, SendAnnounceRequest, ReceiveAnnounceResponse };

  /*! \brief A connection ID is a 64-bit number that is used to identify a connection to a tracker. It is sent by the tracker in response to
   * a connect request. A client can use a connection ID until one minute after it has received it.
   */
  class ConnectionId {
  public:
    ConnectionId() : id(0) {}
    ConnectionId(std::uint64_t id) : id(id), receive_time(std::chrono::steady_clock::now()) {}

    std::uint64_t GetId() const { return id; }
    bool IsValid() const {
      const auto now = std::chrono::steady_clock::now();
      const auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - receive_time);
      return duration.count() < 60;
    }

  private:
    std::uint64_t id;
    std::chrono::steady_clock::time_point receive_time;
  };

  std::atomic_bool cancel_flag_;
  const network::URL url_;
  std::shared_ptr<const tracker::AnnounceParameters> parameters_;
  State state_;
  network::Socket socket_;
  ConnectionId connection_id_;
  std::uint32_t transaction_id_;
  ByteVector buffer_;
  std::size_t current_buffer_position_;
  std::chrono::steady_clock::time_point timeout_time_;
  unsigned nr_timeouts_;
};
} // namespace tracker
} // namespace tortoise

#endif // TORTOISE_TRACKER_CONNECTION_HPP