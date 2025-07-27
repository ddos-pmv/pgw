#include "udp_server.h"

#include <fcntl.h>
#include <netinet/in.h>
#include <pgw_utils/logger_config.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <cstring>
#include <iostream>
#include <memory>

#define MAX_EVENTS 1024

namespace protei {

UdpServer::UdpServer(uint16_t port, moodycamel::ConcurrentQueue<Event>& queue)
    : port_(port),
      server_fd_(socket(AF_INET, SOCK_DGRAM, 0)),
      epoll_fd_(epoll_create1(0)),
      running_{false},
      queue_(queue) {
  if (server_fd_ < 0) throw std::runtime_error("no socket can't be created");

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  int opt = 1;
  if (setsockopt(server_fd_.get(), SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) != 0)
    throw std::runtime_error("Error setting options");

#ifdef SO_REUSEPORT
  if (setsockopt(server_fd_.get(), SOL_SOCKET, SO_REUSEPORT, &opt,
                 sizeof(opt)) != 0) {
    std::cerr << "SO_REUSEPORT not supported";
  }
#endif

  // Set server_fd nonblocking
  int flags = fcntl(server_fd_.get(), F_GETFL, 0);
  if (flags == -1)
    throw std::runtime_error("Failed to get socket flgas:" +
                             std::string(strerror(errno)));
  if (fcntl(server_fd_.get(), F_SETFL, flags | O_NONBLOCK) == -1) {
    throw std::runtime_error("Failed to set O_NONBLOCK: " +
                             std::string(strerror(errno)));
  }

  // Bind UdpServer socket to port
  if (bind(server_fd_.get(), reinterpret_cast<struct sockaddr*>(&serverAddress),
           sizeof(serverAddress)) < 0)
    throw std::runtime_error("Error socket binding");

  // Check if epoll was created
  if (epoll_fd_ < 0) throw std::runtime_error("Erroe creating epoll");

  // Setting evpoll_event with server context
  epoll_event ev{};
  ev.events = EPOLLIN | EPOLLET;  // LT by default
  ev.data.fd = server_fd_.get();

  // Add server's fd_ to get accept events
  if (epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, server_fd_.get(), &ev) < 0)
    throw std::runtime_error("Error adding UdpServer socket to epoll");
}

void UdpServer::start() {
  if (running_.exchange(true)) {
    return;
  }
  io_thread_ = std::thread(&UdpServer::epoll_loop, this);
  UDP_LOG_INFO("Udp Server started with epoll");
}

void UdpServer::epoll_loop() {
  std::array<epoll_event, MAX_EVENTS> events;
  std::vector<uint8_t> buffer(BUFFER_SIZE);

  while (running_.load()) {
    UDP_LOG_TRACE("Epoll waiting for packet");
    int event_count =
        epoll_wait(epoll_fd_.get(), events.data(), MAX_EVENTS, -1);

    if (event_count < 0) {
      if (errno == EINTR) {
        continue;
      }
      UDP_LOG_ERROR("epoll_wait error: {}", strerror(errno));
      break;
    }

    for (int i = 0; i < event_count; i++) {
      const auto& event = events[i];
      if (event.data.fd == server_fd_.get()) {
        if (event.events & EPOLLIN) {
          // Read incoming packets
          process_incoming_packets(buffer);
        }
        if (event.events & (EPOLLERR | EPOLLHUP)) {
          UDP_LOG_ERROR("Socket errr in epoll: {}", strerror(errno));
        }
      }
    }
  }
}

void UdpServer::process_incoming_packets(std::vector<uint8_t>& buffer) {
  sockaddr_in client_addr{};
  socklen_t client_len = sizeof(client_addr);

  while (running_.load()) {
    ssize_t recv =
        recvfrom(server_fd_.get(), buffer.data(), buffer.size(), 0,
                 reinterpret_cast<sockaddr*>(&client_addr), &client_len);

    if (recv < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        break;  // No more data
      }
      if (errno == EINTR) {
        continue;  // Retry
      }
    }

    UdpEvent udp_event;
    udp_event.data =
        std::vector<uint8_t>(buffer.begin(), buffer.begin() + recv);
    udp_event.client_addr = client_addr;
    udp_event.response_callback = [this](const std::string& response,
                                         const sockaddr_in& addr) {
      send_response(response, addr);
    };

    if (!queue_.try_enqueue(std::move(udp_event))) {
      UDP_LOG_ERROR("Failed to enqueue UDP event - queue full");
      send_response("rejected", client_addr);
    } else {
      UDP_LOG_TRACE("Enqueued UDP event with {} bytes", recv);
    }
  }
}
void UdpServer::send_response(const std::string& response,
                              const sockaddr_in& client_addr) {
  ssize_t sent = sendto(server_fd_.get(), response.c_str(), response.length(),
                        0, reinterpret_cast<const sockaddr*>(&client_addr),
                        sizeof(client_addr));

  if (sent < 0) {
    UDP_LOG_ERROR("Failed to send response: {}", strerror(errno));
  } else if (static_cast<size_t>(sent) != response.length()) {
    UDP_LOG_WARN("Partial response sent: {}/{} bytes", sent, response.length());
  } else {
    UDP_LOG_TRACE("Sent response '{}' ({} bytes) to client", response, sent);
  }
}
}  // namespace protei
