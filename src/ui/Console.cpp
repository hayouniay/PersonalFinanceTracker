#include "ui/Console.hpp"
#include "core/Category.hpp"
#include "services/CsvService.hpp"
#include "services/ReportService.hpp"
#include "utils/Date.hpp"
#include "utils/Format.hpp"
#include <iostream>
#include <limits>

Console::Console(FinanceManager &m) : manager_(m) {}
void Console::printMenu() const {
  std::cout
      << "\n=== Personal Finance Tracker ===\n1. Add income\n2. Add "
         "expense\n3. List transactions\n4. Monthly summary\n5. Set budget\n6. "
         "List budgets\n7. Search transactions\n8. Add account\n9. List "
         "accounts\n10. Export transactions CSV\n11. Import transactions "
         "CSV\n12. Savings goals\n13. Dashboard\n14. Generate monthly chart "
         "(SVG)\n15. Transfer between accounts\n16. Recurring "
         "transactions\n17. Forecast\n18. Financial analytics\n0. Exit\n> ";
}
static void clearInput() {
  std::cin.clear();
  std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
}
static std::string readLine(const char *prompt) {
  std::cout << prompt;
  std::string s;
  std::getline(std::cin >> std::ws, s);
  return s;
}
void Console::run() {
  for (;;) {
    printMenu();
    int c;
    if (!(std::cin >> c)) {
      clearInput();
      continue;
    }
    try {
      if (c == 0)
        break;
      if (c == 8) {
        auto n = readLine("Account name: ");
        std::cout << "Created account #" << manager_.addAccount(n) << "\n";
      } else if (c == 9) {
        for (auto &a : manager_.listAccounts())
          std::cout << a.id << " | " << a.name << " | " << money(a.balance)
                    << "\n";
      } else if (c == 1 || c == 2) {
        int aid;
        double amount;
        std::string cat, desc, date;
        std::cout << "Account id: ";
        std::cin >> aid;
        std::cout << "Amount: ";
        std::cin >> amount;
        std::cout << "Category: ";
        std::cin >> cat;
        desc = readLine("Description: ");
        date =
            readLine((std::string("Date [") + Date::today() + "]: ").c_str());
        if (date.empty())
          date = Date::today();
        manager_.addTransaction(aid, amount, cat, desc, date,
                                c == 1 ? TransactionType::Income
                                       : TransactionType::Expense);
        std::cout << "Saved.\n";
      } else if (c == 3) {
        for (auto &t : manager_.listTransactions())
          std::cout << t.id() << " | A" << t.accountId() << " | " << t.date()
                    << " | " << toString(t.type()) << " | " << t.category()
                    << " | " << money(t.amount()) << " | " << t.description()
                    << "\n";
      } else if (c == 4) {
        std::string m;
        std::cout << "Month (YYYY-MM): ";
        std::cin >> m;
        double in = manager_.monthlyIncome(m),
               out = manager_.monthlyExpenses(m);
        std::cout << "Income: " << money(in) << "\nExpenses: " << money(out)
                  << "\nNet: " << money(in - out) << "\n";
      } else if (c == 5) {
        std::string cat;
        double lim;
        std::cout << "Category: ";
        std::cin >> cat;
        std::cout << "Monthly limit: ";
        std::cin >> lim;
        manager_.setBudget(cat, lim);
        std::cout << "Budget saved.\n";
      } else if (c == 6) {
        std::string m;
        std::cout << "Month (YYYY-MM): ";
        std::cin >> m;
        for (auto &b : manager_.listBudgets())
          std::cout << b.category << " | limit " << money(b.monthlyLimit)
                    << " | spent "
                    << money(manager_.categorySpent(m, b.category)) << "\n";
      } else if (c == 7) {
        auto term = readLine("Search: ");
        for (auto &t : manager_.searchTransactions(term))
          std::cout << t.date() << " | " << t.category() << " | "
                    << money(t.amount()) << " | " << t.description() << "\n";
      } else if (c == 10) {
        auto path = readLine("CSV output path: ");
        CsvService::exportTransactions(path, manager_.listTransactions());
        std::cout << "Exported.\n";
      } else if (c == 11) {
        auto path = readLine("CSV input path: ");
        int accountId;
        std::cout << "Target account id: ";
        std::cin >> accountId;
        auto txs = CsvService::importTransactions(path);
        int count = 0;
        for (const auto &t : txs) {
          manager_.addTransaction(accountId, t.amount(), t.category(),
                                  t.description(), t.date(), t.type());
          ++count;
        }
        std::cout << "Imported " << count << " transactions.\n";
      } else if (c == 12) {
        for (;;) {
          std::cout << "\nGoals: 1 Add 2 List 3 Update 4 Delete 0 Back\n> ";
          int g;
          std::cin >> g;
          if (g == 0)
            break;
          if (g == 1) {
            auto n = readLine("Name: ");
            double target;
            std::cout << "Target amount: ";
            std::cin >> target;
            auto d = readLine("Deadline (YYYY-MM-DD, optional): ");
            std::cout << "Created goal #" << manager_.addGoal(n, target, d)
                      << "\n";
          } else if (g == 2) {
            for (auto &x : manager_.listGoals())
              std::cout << x.id << " | " << x.name << " | "
                        << money(x.currentAmount) << " / "
                        << money(x.targetAmount) << " | " << x.deadline << "\n";
          } else if (g == 3) {
            int id;
            double amount;
            std::cout << "Goal id: ";
            std::cin >> id;
            std::cout << "Current amount: ";
            std::cin >> amount;
            manager_.updateGoal(id, amount);
          } else if (g == 4) {
            int id;
            std::cout << "Goal id: ";
            std::cin >> id;
            manager_.deleteGoal(id);
          }
        }
      } else if (c == 13) {
        std::string m;
        std::cout << "Month (YYYY-MM): ";
        std::cin >> m;
        std::cout << ReportService::dashboard(manager_, m);
      } else if (c == 14) {
        std::string m, path;
        std::cout << "Month (YYYY-MM): ";
        std::cin >> m;
        path = readLine("SVG output path: ");
        ReportService::generateMonthlyChart(manager_, m, path);
        std::cout << "Chart written to " << path << "\n";
      } else if (c == 15) {
        int from, to;
        double amount;
        std::cout << "From account id: ";
        std::cin >> from;
        std::cout << "To account id: ";
        std::cin >> to;
        std::cout << "Amount: ";
        std::cin >> amount;
        auto d =
            readLine((std::string("Date [") + Date::today() + "]: ").c_str());
        if (d.empty())
          d = Date::today();
        manager_.transferFunds(from, to, amount, d);
        std::cout << "Transfer completed.\n";
      } else if (c == 16) {
        for (;;) {
          std::cout << "\nRecurring: 1 Add 2 List 3 Activate/Deactivate 4 "
                       "Process 0 Back\n> ";
          int r;
          std::cin >> r;
          if (r == 0)
            break;
          if (r == 1) {
            int aid;
            double amount;
            std::string cat, desc, freq, date;
            std::cout << "Account id: ";
            std::cin >> aid;
            std::cout << "Amount: ";
            std::cin >> amount;
            std::cout << "Category: ";
            std::cin >> cat;
            desc = readLine("Description: ");
            std::cout << "Frequency (daily/weekly/monthly): ";
            std::cin >> freq;
            date = readLine("First date (YYYY-MM-DD): ");
            std::cout << "Created recurring #"
                      << manager_.addRecurringTransaction(
                             aid, amount, cat, desc, TransactionType::Expense,
                             freq, date)
                      << "\n";
          } else if (r == 2) {
            for (auto &x : manager_.listRecurringTransactions())
              std::cout << x.id << " | A" << x.accountId << " | "
                        << toString(x.type) << " | " << x.frequency
                        << " | next " << x.nextDate << " | "
                        << (x.active ? "active" : "paused") << " | "
                        << money(x.amount) << " | " << x.description << "\n";
          } else if (r == 3) {
            int id;
            std::cout << "Recurring id: ";
            std::cin >> id;
            int active;
            std::cout << "Active? (1/0): ";
            std::cin >> active;
            manager_.setRecurringTransactionActive(id, active != 0);
          } else if (r == 4) {
            auto d = readLine(
                (std::string("Process through [") + Date::today() + "]: ")
                    .c_str());
            if (d.empty())
              d = Date::today();
            std::cout << "Created " << manager_.processRecurringTransactions(d)
                      << " transaction(s).\n";
          }
        }
      } else if (c == 17) {
        std::string m;
        int h;
        std::cout << "Forecast month (YYYY-MM): ";
        std::cin >> m;
        std::cout << "History months [3]: ";
        std::cin >> h;
        std::cout << ReportService::forecastReport(manager_, m, h);
      } else if (c == 18) {
        std::string m;
        std::cout << "Month (YYYY-MM): ";
        std::cin >> m;
        std::cout << manager_.financialAnalytics(m);
      } else
        std::cout << "Unknown option.\n";
    } catch (const std::exception &e) {
      std::cout << "Error: " << e.what() << "\n";
    }
  }
}
