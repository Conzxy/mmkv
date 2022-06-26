#include <iostream>
#include <vector>
#include <string>

int main() {
  while (1) {
    std::string cmd;

    std::vector<std::string> args;

    std::cin >> cmd;

    if (cmd == "stradd") {
      args.resize(3);
      std::cin >> args[0];
      std::cin >> args[1];
      std::cin >> args[2];
    }

    for (auto const& arg : args) {
      std::cout << arg << " ";
    }

    std::cout << std::endl;
  }
}
