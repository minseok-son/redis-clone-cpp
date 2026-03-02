#include "../include/CommandRegistry.h"
#include <algorithm>

CommandRegistry::CommandRegistry() {
    registry_["PING"] = [](const auto&, auto&) -> std::string { 
        return "+PONG\r\n"; 
    };

    registry_["SET"] = [](const auto& args, auto& store) -> std::string {
        if (args.size() < 2) return "-ERR wrong number of arguments\r\n";
        store[args[0]] = args[1];
        return "+OK\r\n";
    };

    registry_["GET"] = [](const auto& args, auto& store) -> std::string {
        if (args.size() < 1) return "-ERR wrong number of arguments\r\n";
        auto it = store.find(args[0]);
        if (it != store.end()) {
            return "$" + std::to_string(it->second.size()) + "\r\n" + it->second + "\r\n";
        }
        return "$-1\r\n";
    };
}

std::string CommandRegistry::execute(const std::string& cmd_name, const std::vector<std::string>& args, KVStore& store) {
    std::string upper_name = cmd_name;
    std::transform(upper_name.begin(), upper_name.end(), upper_name.begin(), ::toupper);
    auto it = registry_.find(upper_name);
    if (it != registry_.end()) {
        return it->second(args, store);
    }
    return "-ERR unknown command '" + cmd_name + "'\r\n";
}