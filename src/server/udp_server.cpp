#include "udp_server.h"

#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>

#include <iostream>
#include <memory>

#define MAX_EVENTS 1024

namespace protei {

UdpServer::UdpServer(uint16_t port, moodycamel::ConcurrentQueue<Event>& queue_)
    : port_(port),
      server_fd_(socket(AF_INET, SOCK_DGRAM, 0)),
      epoll_fd_(epoll_create1(0)) {
  if (server_fd_ < 0) throw std::runtime_error("no socket can't be created");

  sockaddr_in serverAddress;
  serverAddress.sin_family = AF_INET;
  serverAddress.sin_port = htons(port);
  serverAddress.sin_addr.s_addr = INADDR_ANY;

  int opt = 1;
  if (setsockopt(server_fd_.get(), SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) != 0)
    throw std::runtime_error("Error setting options");

  // Bind UdpServer socket to port
  if (bind(server_fd_.get(), reinterpret_cast<struct sockaddr*>(&serverAddress),
           sizeof(serverAddress)) < 0)
    throw std::runtime_error("Error socket binding");

  // Check if epoll was created
  if (epoll_fd_ < 0) throw std::runtime_error("Erroe creating epoll");

  // Setting evpoll_event with server context
  epoll_event ev{};
  ev.events = EPOLLIN;  // LT by default
  ev.data.fd = server_fd_.get();

  // Add server's fd_ to get accept events
  if (epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, server_fd_.get(), &ev) < 0)
    throw std::runtime_error("Error adding UdpServer socket to epoll");
}

void UdpServer::start() {
  epoll_event events[MAX_EVENTS];
  while (!stop_) {
    int event_count = epoll_wait(epoll_fd_.get(), events, MAX_EVENTS, -1);

    if (event_count < 0 & errno != EINTR)
      std::runtime_error("Error in epoll_wait");
  }
}

}  // namespace protei
