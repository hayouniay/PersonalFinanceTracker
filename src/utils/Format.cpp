#include "utils/Format.hpp"
#include <iomanip>
#include <sstream>
std::string money(double v) {
  std::ostringstream o;
  o << std::fixed << std::setprecision(2) << v;
  return o.str();
}
