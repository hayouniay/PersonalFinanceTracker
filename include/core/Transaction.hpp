#pragma once
#include <string>

enum class TransactionType { Income, Expense };

std::string toString(TransactionType type);
TransactionType transactionTypeFromString(const std::string &value);

class Transaction {
public:
  Transaction() = default;
  Transaction(int id, int accountId, double amount, std::string category,
              std::string description, std::string date, TransactionType type);

  int id() const noexcept;
  int accountId() const noexcept;
  double amount() const noexcept;
  const std::string &category() const noexcept;
  const std::string &description() const noexcept;
  const std::string &date() const noexcept;
  TransactionType type() const noexcept;

private:
  int id_{0};
  int accountId_{0};
  double amount_{0.0};
  std::string category_;
  std::string description_;
  std::string date_;
  TransactionType type_{TransactionType::Expense};
};
