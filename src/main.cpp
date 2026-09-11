#include "core/FinanceManager.hpp"
#include "ui/Console.hpp"
#include <iostream>
int main(int argc, char **argv) {
  try {
    std::string db = (argc > 1 ? argv[1] : "data/finance.db");
    FinanceManager manager(db);
    manager.initialize();
    Console(manager).run();
    return 0;
  } catch (const std::exception &e) {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
  }
}
