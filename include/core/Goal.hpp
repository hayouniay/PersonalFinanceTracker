#pragma once
#include <string>

struct Goal {
  int id{0};
  std::string name;
  double targetAmount{0.0};
  double currentAmount{0.0};
  std::string deadline;
};
