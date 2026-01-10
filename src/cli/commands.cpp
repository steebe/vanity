#include "command_registry.hpp"
#include <memory>

namespace vanity
{
  std::unique_ptr<Command> create_add_border_command();
  std::unique_ptr<Command> create_inspect_command();
  std::unique_ptr<Command> create_instafy_command();

  void register_all_commands(CommandRegistry &registry)
  {
    registry.register_command(create_add_border_command());
    registry.register_command(create_inspect_command());
    registry.register_command(create_instafy_command());
  }

} // namespace vanity
