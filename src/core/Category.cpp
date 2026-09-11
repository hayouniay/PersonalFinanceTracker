#include "core/Category.hpp"
#include <algorithm>
namespace Category {
bool isValid(const std::string &c) {
  return std::find(Defaults.begin(), Defaults.end(), c) != Defaults.end();
}
} // namespace Category
