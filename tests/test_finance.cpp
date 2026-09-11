#include "core/FinanceManager.hpp"
#include "services/CsvService.hpp"
#include "services/ReportService.hpp"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
int main() {
  const std::string p = "test_finance.db";
  std::filesystem::remove(p);
  std::filesystem::remove("test_transactions.csv");
  std::filesystem::remove("test_chart.svg");
  FinanceManager f(p);
  f.initialize();
  auto a = f.listAccounts();
  assert(a.size() == 1);
  int second = f.addAccount("Savings");
  assert(f.listAccounts().size() == 2);
  f.addTransaction(a[0].id, 100, "Food", "salary", "2026-07-01",
                   TransactionType::Income);
  f.addTransaction(a[0].id, 25, "Food", "lunch, cafe", "2026-08-02",
                   TransactionType::Expense);
  f.addTransaction(a[0].id, 50, "Housing", "rent", "2026-08-03",
                   TransactionType::Expense);
  f.addTransaction(a[0].id, 110, "Food", "salary", "2026-09-01",
                   TransactionType::Income);
  f.addTransaction(a[0].id, 30, "Food", "groceries", "2026-09-02",
                   TransactionType::Expense);
  assert(f.monthlyIncome("2026-09") == 110);
  assert(f.monthlyExpenses("2026-09") == 30);
  assert(f.categorySpent("2026-09", "Food") == 30);
  f.setBudget("Food", 200);
  assert(f.listBudgets().size() == 1);
  assert(f.searchTransactions("lunch").size() == 1);
  int goal = f.addGoal("Emergency fund", 1000, "2026-12-31");
  assert(f.listGoals().size() == 1);
  f.updateGoal(goal, 150);
  assert(f.listGoals()[0].currentAmount == 150);
  f.deleteGoal(goal);
  assert(f.listGoals().empty());
  auto tx = f.listTransactions();
  CsvService::exportTransactions("test_transactions.csv", tx);
  auto imported = CsvService::importTransactions("test_transactions.csv");
  assert(imported.size() == tx.size());
  auto dash = ReportService::dashboard(f, "2026-09");
  assert(dash.find("FINANCIAL DASHBOARD") != std::string::npos);
  ReportService::generateMonthlyChart(f, "2026-09", "test_chart.svg");
  assert(std::filesystem::exists("test_chart.svg"));
  f.transferFunds(a[0].id, second, 20, "2026-09-05", "Savings transfer");
  assert(f.listAccounts()[0].balance == 85);
  assert(f.listAccounts()[1].balance == 20);
  int recurring = f.addRecurringTransaction(a[0].id, 10, "Food", "Subscription",
                                            TransactionType::Expense, "monthly",
                                            "2026-09-06");
  assert(f.listRecurringTransactions(true).size() == 1);
  assert(f.processRecurringTransactions("2026-11-06") == 3);
  assert(f.monthlyExpenses("2026-09") == 60);
  assert(f.monthlyExpenses("2026-10") == 10);
  assert(f.monthlyExpenses("2026-11") == 10);
  assert(f.listRecurringTransactions()[0].nextDate == "2026-12-06");
  f.setRecurringTransactionActive(recurring, false);
  assert(!f.listRecurringTransactions()[0].active);
  assert(f.forecastMonthlyExpenses("2026-10", 2) == 67.5);
  assert(f.forecastMonthlyIncome("2026-10", 2) == 65);
  auto analytics = f.financialAnalytics("2026-09");
  assert(analytics.find("FINANCIAL ANALYTICS") != std::string::npos);
  auto forecast = ReportService::forecastReport(f, "2026-10", 2);
  assert(forecast.find("FORECAST") != std::string::npos);
  std::filesystem::remove(p);
  std::filesystem::remove("test_transactions.csv");
  std::filesystem::remove("test_chart.svg");
  std::cout << "All phase 4 tests passed\n";
}
