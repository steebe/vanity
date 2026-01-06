#ifndef VANITY_COMMAND_REGISTRY_HPP
#define VANITY_COMMAND_REGISTRY_HPP

#include <map>
#include <memory>
#include <string>

namespace vanity
{

  struct CommandResult
  {
    int exit_code;
    std::string message;
  };

  class Command
  {
  public:
    virtual ~Command() = default;

    // Execute the command with given arguments
    // argc/argv are shifted: argv[0] is the command name
    virtual CommandResult execute(int argc, char *argv[]) = 0;

    virtual void print_usage(const char *program_name) const = 0;
    virtual const char *name() const = 0;
    virtual const char *description() const = 0;
  };

  class CommandRegistry
  {
  public:
    CommandRegistry() = default;
    void register_command(std::unique_ptr<Command> cmd);
    Command *find_command(const std::string &name) const;
    void print_available_commands() const;

  private:
    std::map<std::string, std::unique_ptr<Command>> commands_;
  };

} // namespace vanity

#endif // VANITY_COMMAND_REGISTRY_HPP
