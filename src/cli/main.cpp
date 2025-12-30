#include "command_registry.hpp"
#include <iostream>

namespace vanity {
    void register_all_commands(CommandRegistry& registry);
}

void print_vanity_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <command> [options]\n";
    std::cout << "\n";
    std::cout << "Vanity - Experimental image processing toolkit\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    using namespace vanity;

    std::cout << "vanity  Copyright (C) 2025 steebe (steve@stevebass.me)\n";
    std::cout << "    This program comes with ABSOLUTELY NO WARRANTY.\n\n";

    if (argc < 2) {
        print_vanity_usage(argv[0]);
        CommandRegistry registry;
        register_all_commands(registry);
        registry.print_available_commands();
        return 1;
    }

    std::string subcommand = argv[1];

    // Handle help command
    if (subcommand == "--help" || subcommand == "-h" || subcommand == "help") {
        print_vanity_usage(argv[0]);
        CommandRegistry registry;
        register_all_commands(registry);
        registry.print_available_commands();
        return 0;
    }

    // Create registry and register all commands
    CommandRegistry registry;
    register_all_commands(registry);

    // Find and execute command
    Command* cmd = registry.find_command(subcommand);

    if (!cmd) {
        std::cerr << "Error: Unknown command '" << subcommand << "'\n\n";
        print_vanity_usage(argv[0]);
        registry.print_available_commands();
        return 1;
    }

    // Shift arguments: "vanity add_border ..." becomes "add_border ..."
    CommandResult result = cmd->execute(argc - 1, argv + 1);

    if (!result.message.empty()) {
        std::cerr << result.message << "\n";
    }

    return result.exit_code;
}
