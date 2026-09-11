#include "core/FinanceManager.hpp"
#include "core/Category.hpp"
#include "storage/Database.hpp"
#include <algorithm>
#include <iomanip>
#include <map>
#include <memory>
#include <sqlite3.h>
#include <sstream>
#include <stdexcept>

struct FinanceManager::Impl {
  Database db;
  explicit Impl(const std::string &p) : db(p) {}
};
static void bindText(sqlite3_stmt *s, int i, const std::string &v) {
  if (sqlite3_bind_text(s, i, v.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
    throw std::runtime_error("SQLite bind failed");
}
static void check(int rc, sqlite3 *db) {
  if (rc != SQLITE_OK && rc != SQLITE_DONE && rc != SQLITE_ROW)
    throw std::runtime_error(sqlite3_errmsg(db));
}

static int monthIndex(const std::string &d) {
  return std::stoi(d.substr(0, 4)) * 12 + std::stoi(d.substr(5, 2)) - 1;
}
static std::string addDays(const std::string &date, int days) {
  int y = std::stoi(date.substr(0, 4)), m = std::stoi(date.substr(5, 2)),
      d = std::stoi(date.substr(8, 2));
  auto leap = [](int yy) {
    return yy % 4 == 0 && (yy % 100 != 0 || yy % 400 == 0);
  };
  auto dim = [&](int yy, int mm) {
    static const int a[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    return mm == 2 ? a[1] + (leap(yy) ? 1 : 0) : a[mm - 1];
  };
  while (days-- > 0) {
    if (++d > dim(y, m)) {
      d = 1;
      if (++m > 12) {
        m = 1;
        ++y;
      }
    }
  }
  std::ostringstream o;
  o << std::setw(4) << std::setfill('0') << y << '-' << std::setw(2) << m << '-'
    << std::setw(2) << d;
  return o.str();
}
static std::string advanceDate(const std::string &date,
                               const std::string &frequency) {
  if (frequency == "daily")
    return addDays(date, 1);
  if (frequency == "weekly")
    return addDays(date, 7);
  int y = std::stoi(date.substr(0, 4)), m = std::stoi(date.substr(5, 2)),
      d = std::stoi(date.substr(8, 2));
  if (++m > 12) {
    m = 1;
    ++y;
  }
  auto leap = [](int yy) {
    return yy % 4 == 0 && (yy % 100 != 0 || yy % 400 == 0);
  };
  static const int dm[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int maxd = dm[m - 1] + (m == 2 && leap(y));
  d = std::min(d, maxd);
  std::ostringstream o;
  o << std::setw(4) << std::setfill('0') << y << '-' << std::setw(2) << m << '-'
    << std::setw(2) << d;
  return o.str();
}

FinanceManager::FinanceManager(const std::string &p) : impl_(new Impl(p)) {}
FinanceManager::~FinanceManager() { delete impl_; }
void FinanceManager::initialize() {
  impl_->db.execute(R"sql(
CREATE TABLE IF NOT EXISTS accounts(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL UNIQUE,balance REAL NOT NULL DEFAULT 0);
CREATE TABLE IF NOT EXISTS transactions(id INTEGER PRIMARY KEY AUTOINCREMENT,account_id INTEGER NOT NULL,amount REAL NOT NULL CHECK(amount>0),category TEXT NOT NULL,description TEXT NOT NULL,date TEXT NOT NULL,type TEXT NOT NULL CHECK(type IN ('income','expense')),FOREIGN KEY(account_id) REFERENCES accounts(id) ON DELETE CASCADE);
CREATE TABLE IF NOT EXISTS budgets(id INTEGER PRIMARY KEY AUTOINCREMENT,category TEXT NOT NULL UNIQUE,monthly_limit REAL NOT NULL CHECK(monthly_limit>=0));
CREATE TABLE IF NOT EXISTS goals(id INTEGER PRIMARY KEY AUTOINCREMENT,name TEXT NOT NULL,target_amount REAL NOT NULL,current_amount REAL NOT NULL DEFAULT 0,deadline TEXT);
CREATE TABLE IF NOT EXISTS recurring_transactions(id INTEGER PRIMARY KEY AUTOINCREMENT,account_id INTEGER NOT NULL,amount REAL NOT NULL CHECK(amount>0),category TEXT NOT NULL,description TEXT NOT NULL,type TEXT NOT NULL CHECK(type IN ('income','expense')),frequency TEXT NOT NULL CHECK(frequency IN ('daily','weekly','monthly')),next_date TEXT NOT NULL,active INTEGER NOT NULL DEFAULT 1,FOREIGN KEY(account_id) REFERENCES accounts(id) ON DELETE CASCADE);
)sql");
  if (listAccounts().empty())
    addAccount("Main");
}
int FinanceManager::addAccount(const std::string &name) {
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(),
                           "INSERT INTO accounts(name,balance) VALUES(?,0);",
                           -1, &s, nullptr),
        impl_->db.handle());
  bindText(s, 1, name);
  int rc = sqlite3_step(s);
  if (rc != SQLITE_DONE) {
    std::string e = sqlite3_errmsg(impl_->db.handle());
    sqlite3_finalize(s);
    throw std::runtime_error(e);
  }
  int id = (int)sqlite3_last_insert_rowid(impl_->db.handle());
  sqlite3_finalize(s);
  return id;
}
std::vector<Account> FinanceManager::listAccounts() const {
  std::vector<Account> r;
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(),
                           "SELECT id,name,balance FROM accounts ORDER BY id;",
                           -1, &s, nullptr),
        impl_->db.handle());
  while (sqlite3_step(s) == SQLITE_ROW)
    r.push_back({sqlite3_column_int(s, 0),
                 (const char *)sqlite3_column_text(s, 1),
                 sqlite3_column_double(s, 2)});
  sqlite3_finalize(s);
  return r;
}

void FinanceManager::transferFunds(int fromAccountId, int toAccountId,
                                   double amount, const std::string &date,
                                   const std::string &description) {
  if (fromAccountId == toAccountId)
    throw std::invalid_argument("Source and destination accounts must differ");
  if (amount <= 0)
    throw std::invalid_argument("Transfer amount must be positive");
  sqlite3 *db = impl_->db.handle();
  impl_->db.execute("BEGIN TRANSACTION;");
  try {
    sqlite3_stmt *q = nullptr;
    check(sqlite3_prepare_v2(db, "SELECT balance FROM accounts WHERE id=?;", -1,
                             &q, nullptr),
          db);
    sqlite3_bind_int(q, 1, fromAccountId);
    double balance = -1;
    if (sqlite3_step(q) == SQLITE_ROW)
      balance = sqlite3_column_double(q, 0);
    sqlite3_finalize(q);
    if (balance < amount)
      throw std::runtime_error("Insufficient funds");
    q = nullptr;
    check(sqlite3_prepare_v2(db, "SELECT id FROM accounts WHERE id=?;", -1, &q,
                             nullptr),
          db);
    sqlite3_bind_int(q, 1, toAccountId);
    bool exists = sqlite3_step(q) == SQLITE_ROW;
    sqlite3_finalize(q);
    if (!exists)
      throw std::runtime_error("Destination account not found");
    const std::string outDesc = description + " -> account " +
                                std::to_string(toAccountId),
                      inDesc = description + " <- account " +
                               std::to_string(fromAccountId);
    for (int i = 0; i < 2; ++i) {
      int aid = i == 0 ? fromAccountId : toAccountId;
      TransactionType type =
          i == 0 ? TransactionType::Expense : TransactionType::Income;
      const std::string &desc = i == 0 ? outDesc : inDesc;
      sqlite3_stmt *ins = nullptr;
      check(sqlite3_prepare_v2(db,
                               "INSERT INTO "
                               "transactions(account_id,amount,category,"
                               "description,date,type) VALUES(?,?,?,?,?,?);",
                               -1, &ins, nullptr),
            db);
      sqlite3_bind_int(ins, 1, aid);
      sqlite3_bind_double(ins, 2, amount);
      bindText(ins, 3, "Other");
      bindText(ins, 4, desc);
      bindText(ins, 5, date);
      bindText(ins, 6, toString(type));
      check(sqlite3_step(ins), db);
      sqlite3_finalize(ins);
      sqlite3_stmt *upd = nullptr;
      check(sqlite3_prepare_v2(
                db, "UPDATE accounts SET balance=balance+? WHERE id=?;", -1,
                &upd, nullptr),
            db);
      sqlite3_bind_double(upd, 1,
                          type == TransactionType::Income ? amount : -amount);
      sqlite3_bind_int(upd, 2, aid);
      check(sqlite3_step(upd), db);
      sqlite3_finalize(upd);
    }
    impl_->db.execute("COMMIT;");
  } catch (...) {
    impl_->db.execute("ROLLBACK;");
    throw;
  }
}

int FinanceManager::addTransaction(int accountId, double amount,
                                   const std::string &category,
                                   const std::string &description,
                                   const std::string &date,
                                   TransactionType type) {
  if (amount <= 0)
    throw std::invalid_argument("Amount must be positive");
  if (!Category::isValid(category))
    throw std::invalid_argument("Invalid category");
  sqlite3 *db = impl_->db.handle();
  impl_->db.execute("BEGIN TRANSACTION;");
  try {
    sqlite3_stmt *s = nullptr;
    check(sqlite3_prepare_v2(db,
                             "INSERT INTO "
                             "transactions(account_id,amount,category,"
                             "description,date,type) VALUES(?,?,?,?,?,?);",
                             -1, &s, nullptr),
          db);
    sqlite3_bind_int(s, 1, accountId);
    sqlite3_bind_double(s, 2, amount);
    bindText(s, 3, category);
    bindText(s, 4, description);
    bindText(s, 5, date);
    bindText(s, 6, toString(type));
    int rc = sqlite3_step(s);
    if (rc != SQLITE_DONE) {
      sqlite3_finalize(s);
      throw std::runtime_error(sqlite3_errmsg(db));
    }
    int id = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(s);
    sqlite3_stmt *u = nullptr;
    check(sqlite3_prepare_v2(
              db, "UPDATE accounts SET balance=balance+? WHERE id=?;", -1, &u,
              nullptr),
          db);
    sqlite3_bind_double(u, 1,
                        type == TransactionType::Income ? amount : -amount);
    sqlite3_bind_int(u, 2, accountId);
    check(sqlite3_step(u), db);
    if (sqlite3_changes(db) == 0) {
      sqlite3_finalize(u);
      throw std::runtime_error("Account not found");
    }
    sqlite3_finalize(u);
    impl_->db.execute("COMMIT;");
    return id;
  } catch (...) {
    impl_->db.execute("ROLLBACK;");
    throw;
  }
}
std::vector<Transaction> FinanceManager::listTransactions(int accountId) const {
  std::vector<Transaction> r;
  std::string q = "SELECT id,account_id,amount,category,description,date,type "
                  "FROM transactions";
  if (accountId)
    q += " WHERE account_id=?";
  q += " ORDER BY date DESC,id DESC;";
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(), q.c_str(), -1, &s, nullptr),
        impl_->db.handle());
  if (accountId)
    sqlite3_bind_int(s, 1, accountId);
  while (sqlite3_step(s) == SQLITE_ROW)
    r.emplace_back(
        sqlite3_column_int(s, 0), sqlite3_column_int(s, 1),
        sqlite3_column_double(s, 2), (const char *)sqlite3_column_text(s, 3),
        (const char *)sqlite3_column_text(s, 4),
        (const char *)sqlite3_column_text(s, 5),
        transactionTypeFromString((const char *)sqlite3_column_text(s, 6)));
  sqlite3_finalize(s);
  return r;
}
std::vector<Transaction>
FinanceManager::searchTransactions(const std::string &term) const {
  std::vector<Transaction> r;
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(
            impl_->db.handle(),
            "SELECT id,account_id,amount,category,description,date,type FROM "
            "transactions WHERE category LIKE ? OR description LIKE ? ORDER BY "
            "date DESC,id DESC;",
            -1, &s, nullptr),
        impl_->db.handle());
  std::string p = "%" + term + "%";
  bindText(s, 1, p);
  bindText(s, 2, p);
  while (sqlite3_step(s) == SQLITE_ROW)
    r.emplace_back(
        sqlite3_column_int(s, 0), sqlite3_column_int(s, 1),
        sqlite3_column_double(s, 2), (const char *)sqlite3_column_text(s, 3),
        (const char *)sqlite3_column_text(s, 4),
        (const char *)sqlite3_column_text(s, 5),
        transactionTypeFromString((const char *)sqlite3_column_text(s, 6)));
  sqlite3_finalize(s);
  return r;
}
static double sum(Database &db, const std::string &m, int a, const char *t) {
  std::string q = "SELECT COALESCE(SUM(amount),0) FROM transactions WHERE date "
                  "LIKE ? AND type=?";
  if (a)
    q += " AND account_id=?";
  q += ";";
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(db.handle(), q.c_str(), -1, &s, nullptr),
        db.handle());
  bindText(s, 1, m + "%");
  bindText(s, 2, t);
  if (a)
    sqlite3_bind_int(s, 3, a);
  double x = 0;
  if (sqlite3_step(s) == SQLITE_ROW)
    x = sqlite3_column_double(s, 0);
  sqlite3_finalize(s);
  return x;
}
double FinanceManager::monthlyIncome(const std::string &m, int a) const {
  return sum(impl_->db, m, a, "income");
}
double FinanceManager::monthlyExpenses(const std::string &m, int a) const {
  return sum(impl_->db, m, a, "expense");
}
void FinanceManager::setBudget(const std::string &c, double limit) {
  if (!Category::isValid(c) || limit < 0)
    throw std::invalid_argument("Invalid budget");
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(),
                           "INSERT INTO budgets(category,monthly_limit) "
                           "VALUES(?,?) ON CONFLICT(category) DO UPDATE SET "
                           "monthly_limit=excluded.monthly_limit;",
                           -1, &s, nullptr),
        impl_->db.handle());
  bindText(s, 1, c);
  sqlite3_bind_double(s, 2, limit);
  check(sqlite3_step(s), impl_->db.handle());
  sqlite3_finalize(s);
}
std::vector<Budget> FinanceManager::listBudgets() const {
  std::vector<Budget> r;
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(
            impl_->db.handle(),
            "SELECT id,category,monthly_limit FROM budgets ORDER BY category;",
            -1, &s, nullptr),
        impl_->db.handle());
  while (sqlite3_step(s) == SQLITE_ROW)
    r.push_back({sqlite3_column_int(s, 0),
                 (const char *)sqlite3_column_text(s, 1),
                 sqlite3_column_double(s, 2)});
  sqlite3_finalize(s);
  return r;
}
double FinanceManager::categorySpent(const std::string &m,
                                     const std::string &c) const {
  sqlite3_stmt *s = nullptr;
  check(
      sqlite3_prepare_v2(impl_->db.handle(),
                         "SELECT COALESCE(SUM(amount),0) FROM transactions "
                         "WHERE date LIKE ? AND category=? AND type='expense';",
                         -1, &s, nullptr),
      impl_->db.handle());
  bindText(s, 1, m + "%");
  bindText(s, 2, c);
  double x = 0;
  if (sqlite3_step(s) == SQLITE_ROW)
    x = sqlite3_column_double(s, 0);
  sqlite3_finalize(s);
  return x;
}

std::vector<std::string> FinanceManager::categories() const {
  return Category::Defaults;
}
int FinanceManager::addGoal(const std::string &name, double targetAmount,
                            const std::string &deadline) {
  if (name.empty() || targetAmount <= 0)
    throw std::invalid_argument("Goal name and positive target are required");
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(
            impl_->db.handle(),
            "INSERT INTO goals(name,target_amount,current_amount,deadline) "
            "VALUES(?,?,0,?);",
            -1, &s, nullptr),
        impl_->db.handle());
  bindText(s, 1, name);
  sqlite3_bind_double(s, 2, targetAmount);
  bindText(s, 3, deadline);
  check(sqlite3_step(s), impl_->db.handle());
  int id = (int)sqlite3_last_insert_rowid(impl_->db.handle());
  sqlite3_finalize(s);
  return id;
}
std::vector<Goal> FinanceManager::listGoals() const {
  std::vector<Goal> r;
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(
            impl_->db.handle(),
            "SELECT id,name,target_amount,current_amount,COALESCE(deadline,'') "
            "FROM goals ORDER BY deadline,id;",
            -1, &s, nullptr),
        impl_->db.handle());
  while (sqlite3_step(s) == SQLITE_ROW)
    r.push_back({sqlite3_column_int(s, 0),
                 (const char *)sqlite3_column_text(s, 1),
                 sqlite3_column_double(s, 2), sqlite3_column_double(s, 3),
                 (const char *)sqlite3_column_text(s, 4)});
  sqlite3_finalize(s);
  return r;
}
void FinanceManager::updateGoal(int goalId, double currentAmount) {
  if (currentAmount < 0)
    throw std::invalid_argument("Current amount cannot be negative");
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(),
                           "UPDATE goals SET current_amount=? WHERE id=?;", -1,
                           &s, nullptr),
        impl_->db.handle());
  sqlite3_bind_double(s, 1, currentAmount);
  sqlite3_bind_int(s, 2, goalId);
  check(sqlite3_step(s), impl_->db.handle());
  if (sqlite3_changes(impl_->db.handle()) == 0) {
    sqlite3_finalize(s);
    throw std::runtime_error("Goal not found");
  }
  sqlite3_finalize(s);
}
void FinanceManager::deleteGoal(int goalId) {
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(), "DELETE FROM goals WHERE id=?;",
                           -1, &s, nullptr),
        impl_->db.handle());
  sqlite3_bind_int(s, 1, goalId);
  check(sqlite3_step(s), impl_->db.handle());
  if (sqlite3_changes(impl_->db.handle()) == 0) {
    sqlite3_finalize(s);
    throw std::runtime_error("Goal not found");
  }
  sqlite3_finalize(s);
}

int FinanceManager::addRecurringTransaction(int accountId, double amount,
                                            const std::string &category,
                                            const std::string &description,
                                            TransactionType type,
                                            const std::string &frequency,
                                            const std::string &nextDate) {
  if (amount <= 0 || !Category::isValid(category))
    throw std::invalid_argument("Invalid recurring transaction");
  if (frequency != "daily" && frequency != "weekly" && frequency != "monthly")
    throw std::invalid_argument("Frequency must be daily, weekly, or monthly");
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(
            impl_->db.handle(),
            "INSERT INTO "
            "recurring_transactions(account_id,amount,category,description,"
            "type,frequency,next_date,active) VALUES(?,?,?,?,?,?,?,1);",
            -1, &s, nullptr),
        impl_->db.handle());
  sqlite3_bind_int(s, 1, accountId);
  sqlite3_bind_double(s, 2, amount);
  bindText(s, 3, category);
  bindText(s, 4, description);
  bindText(s, 5, toString(type));
  bindText(s, 6, frequency);
  bindText(s, 7, nextDate);
  check(sqlite3_step(s), impl_->db.handle());
  int id = (int)sqlite3_last_insert_rowid(impl_->db.handle());
  sqlite3_finalize(s);
  return id;
}
std::vector<RecurringTransaction>
FinanceManager::listRecurringTransactions(bool activeOnly) const {
  std::vector<RecurringTransaction> r;
  std::string q = "SELECT "
                  "id,account_id,amount,category,description,type,frequency,"
                  "next_date,active FROM recurring_transactions";
  if (activeOnly)
    q += " WHERE active=1";
  q += " ORDER BY next_date,id;";
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(impl_->db.handle(), q.c_str(), -1, &s, nullptr),
        impl_->db.handle());
  while (sqlite3_step(s) == SQLITE_ROW)
    r.push_back(
        {sqlite3_column_int(s, 0), sqlite3_column_int(s, 1),
         sqlite3_column_double(s, 2), (const char *)sqlite3_column_text(s, 3),
         (const char *)sqlite3_column_text(s, 4),
         transactionTypeFromString((const char *)sqlite3_column_text(s, 5)),
         (const char *)sqlite3_column_text(s, 6),
         (const char *)sqlite3_column_text(s, 7),
         sqlite3_column_int(s, 8) != 0});
  sqlite3_finalize(s);
  return r;
}
void FinanceManager::setRecurringTransactionActive(int recurringId,
                                                   bool active) {
  sqlite3_stmt *s = nullptr;
  check(sqlite3_prepare_v2(
            impl_->db.handle(),
            "UPDATE recurring_transactions SET active=? WHERE id=?;", -1, &s,
            nullptr),
        impl_->db.handle());
  sqlite3_bind_int(s, 1, active ? 1 : 0);
  sqlite3_bind_int(s, 2, recurringId);
  check(sqlite3_step(s), impl_->db.handle());
  if (sqlite3_changes(impl_->db.handle()) == 0) {
    sqlite3_finalize(s);
    throw std::runtime_error("Recurring transaction not found");
  }
  sqlite3_finalize(s);
}
int FinanceManager::processRecurringTransactions(
    const std::string &throughDate) {
  int count = 0;
  for (auto r : listRecurringTransactions(true)) {
    std::string next = r.nextDate;
    while (next <= throughDate) {
      addTransaction(r.accountId, r.amount, r.category,
                     r.description + " [recurring]", next, r.type);
      next = advanceDate(next, r.frequency);
      ++count;
    }
    sqlite3_stmt *s = nullptr;
    check(sqlite3_prepare_v2(
              impl_->db.handle(),
              "UPDATE recurring_transactions SET next_date=? WHERE id=?;", -1,
              &s, nullptr),
          impl_->db.handle());
    bindText(s, 1, next);
    sqlite3_bind_int(s, 2, r.id);
    check(sqlite3_step(s), impl_->db.handle());
    sqlite3_finalize(s);
  }
  return count;
}

double FinanceManager::forecastMonthlyIncome(const std::string &month,
                                             int historyMonths,
                                             int accountId) const {
  if (historyMonths < 1)
    throw std::invalid_argument("History months must be positive");
  int target = monthIndex(month);
  double sumv = 0;
  int n = 0;
  for (int i = 1; i <= historyMonths; ++i) {
    int idx = target - i;
    int y = idx / 12, m = idx % 12;
    if (m < 0) {
      m += 12;
      --y;
    }
    std::ostringstream o;
    o << std::setw(4) << std::setfill('0') << y << '-' << std::setw(2) << m + 1;
    sumv += monthlyIncome(o.str(), accountId);
    ++n;
  }
  return n ? sumv / n : 0;
}
double FinanceManager::forecastMonthlyExpenses(const std::string &month,
                                               int historyMonths,
                                               int accountId) const {
  if (historyMonths < 1)
    throw std::invalid_argument("History months must be positive");
  int target = monthIndex(month);
  double sumv = 0;
  int n = 0;
  for (int i = 1; i <= historyMonths; ++i) {
    int idx = target - i;
    int y = idx / 12, m = idx % 12;
    if (m < 0) {
      m += 12;
      --y;
    }
    std::ostringstream o;
    o << std::setw(4) << std::setfill('0') << y << '-' << std::setw(2) << m + 1;
    sumv += monthlyExpenses(o.str(), accountId);
    ++n;
  }
  return n ? sumv / n : 0;
}
std::string FinanceManager::financialAnalytics(const std::string &month,
                                               int accountId) const {
  std::map<std::string, double> totals;
  double total = 0;
  for (const auto &t : listTransactions(accountId)) {
    if (t.type() == TransactionType::Expense && t.date().rfind(month, 0) == 0) {
      totals[t.category()] += t.amount();
      total += t.amount();
    }
  }
  std::vector<std::pair<std::string, double>> ranked(totals.begin(),
                                                     totals.end());
  std::sort(ranked.begin(), ranked.end(),
            [](auto &a, auto &b) { return a.second > b.second; });
  std::ostringstream out;
  out << "\n========== FINANCIAL ANALYTICS: " << month << " ==========\n";
  out << "Total expenses: " << std::fixed << std::setprecision(2) << total
      << "\n";
  out << "Categories by spending\n";
  for (auto &x : ranked)
    out << "- " << x.first << ": " << x.second << " ("
        << (total ? x.second * 100 / total : 0) << "%)\n";
  double in = monthlyIncome(month, accountId),
         ex = monthlyExpenses(month, accountId);
  double count = 0;
  for (const auto &t : listTransactions(accountId))
    if (t.type() == TransactionType::Expense && t.date().rfind(month, 0) == 0)
      ++count;
  out << "Average transaction expense: " << (count ? ex / count : 0) << "\n";
  out << "Net cash flow: " << (in - ex) << "\n";
  return out.str();
}
