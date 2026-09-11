#pragma once
#include "core/FinanceManager.hpp"
class Console {
public:
  explicit Console(FinanceManager &manager);
  void run();

private:
  FinanceManager &manager_;
  void printMenu() const;
};
