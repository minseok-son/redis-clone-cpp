#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

using KVStore = std::unordered_map<std::string, std::string>;

using CommandFunc = std::function<std::string(const std::vector<std::string>&, KVStore&)>;

class CommandRegistry {
public:
    CommandRegistry();
    std::string execute(const std::string& cmd_name, const std::vector<std::string>& args, KVStore& store);

private:
    std::unordered_map<std::string, CommandFunc> registry_;

};

#endif