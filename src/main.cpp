#include "../include/Application.hpp"
#include "../include/Config.hpp"

int main(int argc, char *argv[]) {
  auto config = Config::parse(argc, argv);
  if (!config.has_value())
    return 0;

  Application app(std::move(*config));
  return app.run();
}
