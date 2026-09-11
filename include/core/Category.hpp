#pragma once
#include <string>
#include <vector>

namespace Category {
inline const std::vector<std::string> Defaults{
    "Food", "Housing", "Transportation", "Entertainment", "Other"};
bool isValid(const std::string &category);
} // namespace Category
