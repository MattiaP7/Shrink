#include "../include/Application.hpp"
#include "../include/Config.hpp"

/**
 * @brief Parses options and starts the image compression application.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument values.
 * @return Application exit status.
 */
int main(int argc, char *argv[]) {
  auto config = Config::parse(argc, argv);
  if (!config.has_value())
    return 0;

  Application app(std::move(*config));
  return app.run();
}
