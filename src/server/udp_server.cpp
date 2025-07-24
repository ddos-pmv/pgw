#include "udp_server.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>

#include <memory>
#include <iostream>

#define MAX_EVENTS 1024

namespace protei
{

    Server::Server(uint16_t port, int thread_count) : port_(port),
                                                      server_fd_(socket(AF_INET, SOCK_DGRAM, 0)),
                                                      epoll_fd_(epoll_create1(0)),
                                                      stop_(false)
    {
        if (server_fd_ < 0)
            throw std::runtime_error("no socket can't be created");

        sockaddr_in serverAddress;
        serverAddress.sin_family = AF_INET;
        serverAddress.sin_port = htons(port);
        serverAddress.sin_addr.s_addr = INADDR_ANY;

        int opt = 1;
        if (setsockopt(server_fd_.get(), SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) != 0)
            throw std::runtime_error("Error setting options");

        // Bind server socket to port
        if (bind(server_fd_.get(), reinterpret_cast<struct sockaddr *>(&serverAddress),
                 sizeof(serverAddress)) < 0)
            throw std::runtime_error("Error socket binding");

        // Check if epoll was created
        if (epoll_fd_ < 0)
            throw std::runtime_error("Erroe creating epoll");

        // Setting evpoll_event with server context
        epoll_event ev{};
        ev.events = EPOLLIN; // LT by default
        ev.data.fd = server_fd_.get();

        // Add server's fd_ to get accept events
        if (epoll_ctl(epoll_fd_.get(), EPOLL_CTL_ADD, server_fd_.get(), &ev) < 0)
            throw std::runtime_error("Error adding server socket to epoll");

    }

    void Server::start()
    {
        epoll_event events[MAX_EVENTS];
        while (!stop_)
        {
            int event_count = epoll_wait(epoll_fd_.get(), events, MAX_EVENTS, -1);

            if (event_count < 0 & errno != EINTR)
                std::runtime_error("Error in epoll_wait");

            // for (int i = 0; i < event_count; i++)
            // {
            //     UniqueCtx *ctx_ptr = static_cast<UniqueCtx *>(events[i].data.ptr);
            //     if (ctx_ptr->fd() == server_ctx_->fd())
            //     {
            //         try
            //         {
            //             accept_new_connection();
            //         }
            //         catch (const std::exception &e)
            //         {
            //             std::cerr << e.what() << '\n';
            //         }
            //     }
            //     else
            //     {
            //         add_to_queue(ctx_ptr);
            //     }
            // }
        }
    }

    void Server::worker_thread()
    {
        while (!stop_.load(std::memory_order_acquire))
        {
            // UniqueCtx *client_ctx;

            // {
            //     std::unique_lock lock(queue_mtx_);
            //     cv_.wait(lock, [this]()
            //              { return stop_ || !connection_queue_.empty(); });

            //     if (stop_ && connection_queue_.empty())
            //         break;

            //     client_ctx = connection_queue_.front();
            //     connection_queue_.pop();
            // }
            // {
            //     std::unique_lock set_lock(set_mtx_);
            //     // Skip this client if it's even in thread
            //     if (busy_clients_.contains(client_ctx->fd()))
            //         continue;
            //     else
            //     {
            //         busy_clients_.insert(client_ctx->fd());
            //         process_client(client_ctx);
            //     }
            // }
        }
    }

} // namespace protei
