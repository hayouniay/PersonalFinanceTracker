#include "storage/Database.hpp"
#include <sqlite3.h>
#include <stdexcept>
Database::Database(const std::string &path) {
  if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
    std::string e = db_ ? sqlite3_errmsg(db_) : "unknown error";
    if (db_)
      sqlite3_close(db_);
    db_ = nullptr;
    throw std::runtime_error("Cannot open database: " + e);
  }
  execute("PRAGMA foreign_keys = ON;");
}
Database::~Database() {
  if (db_)
    sqlite3_close(db_);
}
sqlite3 *Database::handle() const noexcept { return db_; }
void Database::execute(const std::string &sql) const {
  char *err = nullptr;
  if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
    std::string e = err ? err : "unknown error";
    sqlite3_free(err);
    throw std::runtime_error("SQLite error: " + e);
  }
}
