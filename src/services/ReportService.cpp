#include "services/ReportService.hpp"
#include "core/Category.hpp"
#include "utils/Format.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>

std::string ReportService::dashboard(const FinanceManager &m,
                                     const std::string &month) {
  double income = m.monthlyIncome(month), expense = m.monthlyExpenses(month);
  std::ostringstream out;
  out << "\n========== FINANCIAL DASHBOARD: " << month << " ==========\n";
  out << "Income   : " << money(income) << "\nExpense  : " << money(expense)
      << "\nNet      : " << money(income - expense) << "\n";
  out << "Savings rate: " << std::fixed << std::setprecision(1)
      << (income > 0 ? (income - expense) * 100.0 / income : 0.0) << "%\n\n";
  out << "Category spending\n";
  double maxSpent = 0;
  for (const auto &c : Category::Defaults)
    maxSpent = std::max(maxSpent, m.categorySpent(month, c));
  for (const auto &c : Category::Defaults) {
    double v = m.categorySpent(month, c);
    int bars = maxSpent > 0 ? static_cast<int>(v / maxSpent * 30) : 0;
    out << std::left << std::setw(18) << c << " " << std::string(bars, '#')
        << " " << money(v) << "\n";
  }
  out << "\nAccounts\n";
  for (const auto &a : m.listAccounts())
    out << "- " << a.name << ": " << money(a.balance) << "\n";
  out << "\nSavings goals\n";
  for (const auto &g : m.listGoals()) {
    double pct =
        g.targetAmount > 0 ? g.currentAmount * 100.0 / g.targetAmount : 0;
    out << "- " << g.name << ": " << money(g.currentAmount) << " / "
        << money(g.targetAmount) << " (" << std::fixed << std::setprecision(1)
        << pct << "%)";
    if (!g.deadline.empty())
      out << " by " << g.deadline;
    out << "\n";
  }
  return out.str();
}

std::string ReportService::forecastReport(const FinanceManager &m,
                                          const std::string &month,
                                          int historyMonths) {
  const double in = m.forecastMonthlyIncome(month, historyMonths),
               ex = m.forecastMonthlyExpenses(month, historyMonths);
  std::ostringstream out;
  out << "\n========== FORECAST: " << month << " ==========\n";
  out << "Based on the previous " << historyMonths << " month(s)\n";
  out << "Forecast income  : " << money(in) << "\n";
  out << "Forecast expenses: " << money(ex) << "\n";
  out << "Forecast net     : " << money(in - ex) << "\n";
  out << "Savings rate     : " << std::fixed << std::setprecision(1)
      << (in > 0 ? (in - ex) * 100 / in : 0) << "%\n";
  return out.str();
}

void ReportService::generateMonthlyChart(const FinanceManager &m,
                                         const std::string &month,
                                         const std::string &path) {
  const int width = 900, height = 520, left = 80, bottom = 70, chartW = 760,
            chartH = 360;
  std::ofstream out(path);
  if (!out)
    throw std::runtime_error("Cannot write chart: " + path);
  double in = m.monthlyIncome(month), ex = m.monthlyExpenses(month),
         maxv = std::max({in, ex, 1.0});
  auto h = [&](double v) { return v / maxv * chartH; };
  out << "<svg xmlns='http://www.w3.org/2000/svg' width='" << width
      << "' height='" << height << "' viewBox='0 0 " << width << " " << height
      << "'>\n";
  out << "<rect width='100%' height='100%' fill='white'/>\n<title>Monthly "
         "finance chart "
      << month << "</title>\n";
  out << "<text x='450' y='35' text-anchor='middle' font-family='sans-serif' "
         "font-size='24'>Finance summary "
      << month << "</text>\n";
  out << "<line x1='" << left << "' y1='" << height - bottom << "' x2='"
      << left + chartW << "' y2='" << height - bottom << "' stroke='black'/>\n";
  double vals[2] = {in, ex};
  const char *labels[2] = {"Income", "Expenses"};
  for (int i = 0; i < 2; ++i) {
    double bh = h(vals[i]);
    double x = 220 + i * 300, y = height - bottom - bh;
    out << "<rect x='" << x << "' y='" << y << "' width='140' height='" << bh
        << "' fill='steelblue'/>";
    out << "<text x='" << x + 70 << "' y='" << height - bottom + 30
        << "' text-anchor='middle' font-family='sans-serif'>" << labels[i]
        << "</text>";
    out << "<text x='" << x + 70 << "' y='" << y - 10
        << "' text-anchor='middle' font-family='sans-serif'>" << money(vals[i])
        << "</text>\n";
  }
  out << "</svg>\n";
}
