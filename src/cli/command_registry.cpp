#include "command_registry.hpp"
#include <iostream>

namespace vanity {

void CommandRegistry::register_command(std::unique_ptr<Command> cmd) {
    if (cmd) {
        std::string name = cmd->name();
        commands_[name] = std::move(cmd);
    }
}

Command* CommandRegistry::find_command(const std::string& name) const {
    auto it = commands_.find(name);
    if (it != commands_.end()) {
        return it->second.get();
    }
    return nullptr;
}

void CommandRegistry::print_available_commands() const {
    std::cout << "Available commands:\n";
    for (const auto& pair : commands_) {
        std::cout << "  " << pair.first << " - " << pair.second->description() << "\n";
    }
}

} // namespace vanity
