#include "../include/RedisServer.h"

#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

RedisServer::RedisServer(int port) : listen_fd_(-1), epoll_fd_(-1), port_(port) {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(listen_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd_, SOMAXCONN) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    epoll_fd_= epoll_create1(0);
    if (epoll_fd_ < 0) {
        perror("Epoll create failed");
        exit(EXIT_FAILURE);
    }

    if (listen_fd_ < 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev{};
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &ev);

    std::cout << "Server listening on port 6379..." << std::endl;
}

RedisServer::~RedisServer() {
    stop();
}

void RedisServer::handle_new_connection() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    struct epoll_event ev{};
    int conn_fd = accept4(listen_fd_, (struct sockaddr*)&client_addr, &client_len, SOCK_NONBLOCK);

    ev.events = EPOLLIN;
    ev.data.fd = conn_fd;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, conn_fd, &ev);

    parsers_[conn_fd] = RespParser();
    std::cout << "New client connected: FD " << conn_fd << std::endl;
}

void RedisServer::handle_client_data(int client_fd) {
    char buffer[READ_BUFFER];

    ssize_t bytes_read = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

    if (bytes_read > 0) {
        parsers_[client_fd].feed(buffer, bytes_read); 

        while (auto cmd = parsers_[client_fd].parse()) {
            std::string response;

            if (cmd->command == "PING") {
                response = "+PONG\r\n";
            } else if (cmd->command == "SET" && cmd->args.size() >= 2) {
                kv_store_[cmd->args[0]] = cmd->args[1];
                response = "+OK\r\n";
            } else if (cmd->command == "GET" && cmd->args.size() >= 1) {
                if (kv_store_.count(cmd->args[0])) {
                    std::string val = kv_store_[cmd->args[0]];
                    response = "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
                } else {
                    response = "$-1\r\n";
                }
            } else {
                response = "-ERR unknown command\r\n";
            }

            send(client_fd, response.data(), response.size(), 0);
        }
    } else if (bytes_read == 0) {
        std::cout << "Client FD " << client_fd << " disconnected." << std::endl;
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
        close(client_fd);
        parsers_.erase(client_fd);
    } 
}

void RedisServer::run() {
    struct epoll_event events[MAX_EVENTS];
    is_running_ = true;
    while (is_running_) {
        int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

        if (nfds < 0) {
            if (errno == EINTR) continue;
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd_) {
                handle_new_connection();
            } else {
                handle_client_data(fd);
            }
        }
    }
}

void RedisServer::stop() {
    is_running_ = false;
    if (listen_fd_ != -1) {
        close(listen_fd_);
        listen_fd_ = -1;
    }

    if (epoll_fd_ != -1) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }

    for (auto const& [fd, parser] : parsers_) {
        close(fd);
    }
    parsers_.clear();
    std::cout << "RedisServer stopped and all connections closed." << std::endl;
}