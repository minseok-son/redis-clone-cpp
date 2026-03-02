#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <vector>
#include <unordered_map>
#include "RespParser.h"

class RedisServer {
public:
    RedisServer(int port);
    ~RedisServer();
    void run();
    void stop();
private:
    int listen_fd_;
    int epoll_fd_;
    int port_;
    bool is_running_;
    std::unordered_map<std::string, std::string> kv_store_;
    std::unordered_map<int, RespParser> parsers_;

    void handle_new_connection();
    void handle_client_data(int client_fd);
};

#endif