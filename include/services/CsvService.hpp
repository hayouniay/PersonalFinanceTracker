#pragma once
#include "core/Transaction.hpp"
#include <string>
#include <vector>

class CsvService {
public:
  static void exportTransactions(const std::string &path,
                                 const std::vector<Transaction> &transactions);
  static std::vector<Transaction> importTransactions(const std::string &path);
};
