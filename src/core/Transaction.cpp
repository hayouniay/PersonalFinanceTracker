#include "core/Transaction.hpp"
#include <algorithm>
#include <cctype>
#include <stdexcept>

std::string toString(TransactionType type) {
  return type == TransactionType::Income ? "income" : "expense";
}
TransactionType transactionTypeFromString(const std::string &value) {
  std::string v = value;
  std::transform(v.begin(), v.end(), v.begin(),
                 [](unsigned char c) { return std::tolower(c); });
  if (v == "income")
    return TransactionType::Income;
  if (v == "expense")
    return TransactionType::Expense;
  throw std::invalid_argument("Unknown transaction type: " + value);
}
Transaction::Transaction(int id, int accountId, double amount,
                         std::string category, std::string description,
                         std::string date, TransactionType type)
    : id_(id), accountId_(accountId), amount_(amount),
      category_(std::move(category)), description_(std::move(description)),
      date_(std::move(date)), type_(type) {}
int Transaction::id() const noexcept { return id_; }
int Transaction::accountId() const noexcept { return accountId_; }
double Transaction::amount() const noexcept { return amount_; }
const std::string &Transaction::category() const noexcept { return category_; }
const std::string &Transaction::description() const noexcept {
  return description_;
}
const std::string &Transaction::date() const noexcept { return date_; }
TransactionType Transaction::type() const noexcept { return type_; }
