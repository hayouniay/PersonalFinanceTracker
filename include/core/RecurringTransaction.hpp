#pragma once
#include "core/Transaction.hpp"
#include <string>

struct RecurringTransaction {
  int id{0};
  int accountId{0};
  double amount{0.0};
  std::string category;
  std::string description;
  TransactionType type{TransactionType::Expense};
  std::string frequency{"monthly"};
  std::string nextDate;
  bool active{true};
};
