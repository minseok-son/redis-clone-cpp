#ifndef RESP_PARSER_H
#define RESP_PARSER_H

#include <vector>
#include <string>
#include <optional>

enum class State {
    STATE_IDLE,
    STATE_ARRAY_LEN,
    STATE_BULK_LEN,
    STATE_BULK_DATA,
};

struct RedisCommand {
    std::string command;
    std::vector<std::string> args;
};

class RespParser {
public:
    RespParser() : state(State::STATE_IDLE) {};
    void feed(const char* data, size_t len);
    std::optional<RedisCommand> parse();

private:
    std::string buffer;
    State state;

    int expected_args = -1;
    int current_bulk_len = -1;
    std::vector<std::string> args_accumulator;

    std::optional<std::string> readLine();
};

#endif