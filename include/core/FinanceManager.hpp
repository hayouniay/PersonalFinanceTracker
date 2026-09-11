#pragma once
#include "core/Account.hpp"
#include "core/Budget.hpp"
#include "core/Goal.hpp"
#include "core/RecurringTransaction.hpp"
#include "core/Transaction.hpp"
#include <string>
#include <vector>

class FinanceManager {
public:
  explicit FinanceManager(const std::string &databasePath);
  ~FinanceManager();
  FinanceManager(const FinanceManager &) = delete;
  FinanceManager &operator=(const FinanceManager &) = delete;

  void initialize();
  int addAccount(const std::string &name);
  std::vector<Account> listAccounts() const;
  void transferFunds(int fromAccountId, int toAccountId, double amount,
                     const std::string &date,
                     const std::string &description = "Transfer");
  int addTransaction(int accountId, double amount, const std::string &category,
                     const std::string &description, const std::string &date,
                     TransactionType type);
  std::vector<Transaction> listTransactions(int accountId = 0) const;
  std::vector<Transaction> searchTransactions(const std::string &term) const;
  double monthlyIncome(const std::string &month, int accountId = 0) const;
  double monthlyExpenses(const std::string &month, int accountId = 0) const;
  void setBudget(const std::string &category, double monthlyLimit);
  std::vector<Budget> listBudgets() const;
  double categorySpent(const std::string &month,
                       const std::string &category) const;
  std::vector<std::string> categories() const;
  int addGoal(const std::string &name, double targetAmount,
              const std::string &deadline = "");
  std::vector<Goal> listGoals() const;
  void updateGoal(int goalId, double currentAmount);
  void deleteGoal(int goalId);

  int addRecurringTransaction(int accountId, double amount,
                              const std::string &category,
                              const std::string &description,
                              TransactionType type,
                              const std::string &frequency,
                              const std::string &nextDate);
  std::vector<RecurringTransaction>
  listRecurringTransactions(bool activeOnly = false) const;
  void setRecurringTransactionActive(int recurringId, bool active);
  int processRecurringTransactions(const std::string &throughDate);

  double forecastMonthlyIncome(const std::string &month, int historyMonths = 3,
                               int accountId = 0) const;
  double forecastMonthlyExpenses(const std::string &month,
                                 int historyMonths = 3,
                                 int accountId = 0) const;
  std::string financialAnalytics(const std::string &month,
                                 int accountId = 0) const;

private:
  struct Impl;
  Impl *impl_;
};
