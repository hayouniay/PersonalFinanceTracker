#pragma once
#include <string>

struct Budget {
  int id{0};
  std::string category;
  double monthlyLimit{0.0};
};
