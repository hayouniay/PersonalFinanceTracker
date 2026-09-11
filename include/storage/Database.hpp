#pragma once
#include <string>
struct sqlite3;

class Database {
public:
  explicit Database(const std::string &path);
  ~Database();
  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;
  sqlite3 *handle() const noexcept;
  void execute(const std::string &sql) const;

private:
  sqlite3 *db_{nullptr};
};
