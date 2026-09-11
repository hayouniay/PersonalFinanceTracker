#include "utils/Date.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
namespace Date {
std::string today() {
  auto now = std::chrono::system_clock::now();
  std::time_t t = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
  localtime_r(&t, &tm);
  std::ostringstream o;
  o << std::put_time(&tm, "%Y-%m-%d");
  return o.str();
}
bool isValid(const std::string &v) {
  if (v.size() != 10 || v[4] != '-' || v[7] != '-')
    return false;
  for (size_t i = 0; i < v.size(); ++i)
    if ((i != 4 && i != 7) && !std::isdigit(static_cast<unsigned char>(v[i])))
      return false;
  int m = std::stoi(v.substr(5, 2)), d = std::stoi(v.substr(8, 2));
  return m >= 1 && m <= 12 && d >= 1 && d <= 31;
}
} // namespace Date
