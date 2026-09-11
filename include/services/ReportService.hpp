#pragma once
#include "core/FinanceManager.hpp"
#include <string>

class ReportService {
public:
  static std::string dashboard(const FinanceManager &manager,
                               const std::string &month);
  static std::string forecastReport(const FinanceManager &manager,
                                    const std::string &month,
                                    int historyMonths = 3);
  static void generateMonthlyChart(const FinanceManager &manager,
                                   const std::string &month,
                                   const std::string &outputPath);
};
