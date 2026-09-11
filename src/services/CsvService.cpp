#include "services/CsvService.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

static std::string csvEscape(const std::string &value) {
  bool quote = value.find_first_of(",\"\n\r") != std::string::npos;
  std::string out;
  for (char c : value) {
    if (c == '"')
      out += "\"\"";
    else
      out += c;
  }
  return quote ? "\"" + out + "\"" : out;
}

static std::vector<std::string> parseCsv(const std::string &line) {
  std::vector<std::string> fields;
  std::string field;
  bool quoted = false;
  for (size_t i = 0; i < line.size(); ++i) {
    char c = line[i];
    if (quoted) {
      if (c == '"' && i + 1 < line.size() && line[i + 1] == '"') {
        field += '"';
        ++i;
      } else if (c == '"')
        quoted = false;
      else
        field += c;
    } else if (c == '"')
      quoted = true;
    else if (c == ',') {
      fields.push_back(field);
      field.clear();
    } else
      field += c;
  }
  fields.push_back(field);
  return fields;
}

void CsvService::exportTransactions(const std::string &path,
                                    const std::vector<Transaction> &txs) {
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("Cannot open CSV for writing: " + path);
  out << "id,account_id,amount,category,description,date,type\n";
  for (const auto &t : txs)
    out << t.id() << ',' << t.accountId() << ',' << t.amount() << ','
        << csvEscape(t.category()) << ',' << csvEscape(t.description()) << ','
        << t.date() << ',' << toString(t.type()) << '\n';
}

std::vector<Transaction>
CsvService::importTransactions(const std::string &path) {
  std::ifstream in(path);
  if (!in)
    throw std::runtime_error("Cannot open CSV for reading: " + path);
  std::string line;
  if (!std::getline(in, line))
    return {};
  std::vector<Transaction> result;
  int lineNo = 1;
  while (std::getline(in, line)) {
    ++lineNo;
    if (line.empty())
      continue;
    auto f = parseCsv(line);
    if (f.size() != 7)
      throw std::runtime_error("Invalid CSV line " + std::to_string(lineNo));
    try {
      result.emplace_back(std::stoi(f[0]), std::stoi(f[1]), std::stod(f[2]),
                          f[3], f[4], f[5], transactionTypeFromString(f[6]));
    } catch (const std::exception &e) {
      throw std::runtime_error("Invalid CSV line " + std::to_string(lineNo) +
                               ": " + e.what());
    }
  }
  return result;
}
