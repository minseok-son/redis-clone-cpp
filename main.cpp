#include <iostream>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include "include/RespParser.h"

#define MAX_EVENTS 1024
#define READ_BUFFER 1024

int main() {
    int listen_fd, conn_fd, epoll_fd, nfds;
    struct epoll_event ev, events[MAX_EVENTS];

    std::unordered_map<std::string, std::string> kv_store;
    std::unordered_map<int, RespParser> parsers;

    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(6379);
    socklen_t addrlen = sizeof(address);

    if (bind(listen_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(listen_fd, SOMAXCONN) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    epoll_fd = epoll_create1(0);

    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev);

    std::cout << "Server listening on port 6379..." << std::endl;

    while (true) {
        nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                conn_fd = accept4(listen_fd, (struct sockaddr*)&address, &addrlen, SOCK_NONBLOCK);
                
                ev.events = EPOLLIN;
                ev.data.fd = conn_fd;
                epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_fd, &ev);

                parsers[conn_fd] = RespParser();
                std::cout << "New client connected: FD " << conn_fd << std::endl;
            } else {
                char buffer[READ_BUFFER];

                ssize_t bytes_read = recv(fd, buffer, sizeof(buffer) - 1, 0);

                if (bytes_read > 0) {
                    parsers[fd].feed(buffer, bytes_read); 

                    while (auto cmd = parsers[fd].parse()) {
                        std::string response;

                        if (cmd->command == "PING") {
                            response = "+PONG\r\n";
                        } else if (cmd->command == "SET" && cmd->args.size() >= 2) {
                            kv_store[cmd->args[0]] = cmd->args[1];
                            response = "+OK\r\n";
                        } else if (cmd->command == "GET" && cmd->args.size() >= 1) {
                            if (kv_store.count(cmd->args[0])) {
                                std::string val = kv_store[cmd->args[0]];
                                response = "$" + std::to_string(val.size()) + "\r\n" + val + "\r\n";
                            } else {
                                response = "$-1\r\n";
                            }
                        } else {
                            response = "-ERR unknown command\r\n";
                        }

                        send(fd, response.data(), response.size(), 0);
                    }
                } else if (bytes_read == 0) {
                    // Client closed connection
                    std::cout << "Client FD " << fd << " disconnected." << std::endl;
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
                    close(fd);
                } 
            }
        }
    }

    close(listen_fd);
    return 0;
}