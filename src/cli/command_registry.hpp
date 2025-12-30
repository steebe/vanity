#ifndef VANITY_COMMAND_REGISTRY_HPP
#define VANITY_COMMAND_REGISTRY_HPP

#include <map>
#include <memory>
#include <string>

namespace vanity {

// Result from command execution
struct CommandResult {
    int exit_code;
    std::string message;  // Optional error/info message
};

// Base class for all commands
class Command {
public:
    virtual ~Command() = default;

    // Execute the command with given arguments
    // argc/argv are shifted: argv[0] is the command name
    virtual CommandResult execute(int argc, char* argv[]) = 0;

    // Print usage information
    virtual void print_usage(const char* program_name) const = 0;

    // Get command name
    virtual const char* name() const = 0;

    // Get command description
    virtual const char* description() const = 0;
};

// Registry for managing commands
class CommandRegistry {
public:
    CommandRegistry() = default;

    // Register a command
    void register_command(std::unique_ptr<Command> cmd);

    // Find a command by name (returns nullptr if not found)
    Command* find_command(const std::string& name) const;

    // Print list of available commands
    void print_available_commands() const;

private:
    std::map<std::string, std::unique_ptr<Command>> commands_;
};

} // namespace vanity

#endif // VANITY_COMMAND_REGISTRY_HPP
