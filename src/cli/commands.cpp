#include "command_registry.hpp"
#include <memory>

namespace vanity {

// Forward declarations of command factory functions
std::unique_ptr<Command> create_add_border_command();

// Register all available commands
void register_all_commands(CommandRegistry& registry) {
    registry.register_command(create_add_border_command());
    // Future commands will be registered here
}

} // namespace vanity
