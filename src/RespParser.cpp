#include "../include/RespParser.h"

void RespParser::feed(const char* data, size_t len) {
    buffer.append(data, len);
}

std::optional<std::string> RespParser::readLine() {
    size_t pos = buffer.find("\r\n");
    if (pos == std::string::npos) {
        return std::nullopt;
    }

    std::string line = buffer.substr(0, pos);
    buffer.erase(0, pos + 2);
    return line;
}

std::optional<RedisCommand> RespParser::parse() {
    while (true) {
        if (state == State::STATE_IDLE) {
            if (buffer.empty()) {
                return std::nullopt;
            }

            if (buffer[0] == '*') {
                state = State::STATE_ARRAY_LEN;
            } else {
                buffer.clear();
                return std::nullopt;
            }
        }

        if (state == State::STATE_ARRAY_LEN) {
            auto line = readLine();
            if (!line) {
                return std::nullopt;
            }

            expected_args = std::stoi(line->substr(1));
            args_accumulator.clear();
            state = State::STATE_BULK_LEN;
        }

        if (state == State::STATE_BULK_LEN) {
            if (args_accumulator.size() == (size_t)expected_args) {
                RedisCommand cmd;
                cmd.command = args_accumulator[0];
                for (size_t i = 1; i < args_accumulator.size(); ++i) {
                    cmd.args.push_back(args_accumulator[i]);
                }
                state = State::STATE_IDLE;
                return cmd;
            }

            auto line = readLine();
            if (!line) {
                return std::nullopt;
            }

            current_bulk_len = std::stoi(line->substr(1));
            state = State::STATE_BULK_DATA;
        }

        if (state == State::STATE_BULK_DATA) {
            if (buffer.size() < (size_t)current_bulk_len + 2) {
                return std::nullopt;
            }

            std::string data = buffer.substr(0, current_bulk_len);
            buffer.erase(0, current_bulk_len + 2);

            args_accumulator.push_back(data);
            state = State::STATE_BULK_LEN;
        }
    }
}