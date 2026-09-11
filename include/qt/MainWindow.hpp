#pragma once
#include <QMainWindow>
#include <memory>

class FinanceManager;
class QTableWidget;
class QLabel;
class QComboBox;
class QDateEdit;
class QDoubleSpinBox;
class QLineEdit;
class QSpinBox;
class QTabWidget;

class MainWindow final : public QMainWindow {
  Q_OBJECT
public:
  explicit MainWindow(FinanceManager &manager, QWidget *parent = nullptr);

private slots:
  void refreshAll();
  void addIncome();
  void addExpense();
  void addAccount();
  void transferFunds();
  void exportCsv();
  void importCsv();
  void addBudget();
  void addGoal();
  void updateGoal();
  void deleteGoal();
  void addRecurring();
  void processRecurring();
  void toggleRecurring();
  void refreshDashboard();
  void refreshForecast();
  void generateChart();

private:
  void buildUi();
  void buildDashboardTab();
  void buildTransactionsTab();
  void buildAccountsTab();
  void buildBudgetsTab();
  void buildGoalsTab();
  void buildRecurringTab();
  void buildAnalyticsTab();
  void loadAccounts(QComboBox *combo, bool includeAll = false);
  QString currentMonth() const;
  int selectedId(QTableWidget *table) const;
  void showError(const QString &message);

  FinanceManager &manager_;
  QTabWidget *tabs_{nullptr};
  QLabel *incomeLabel_{nullptr};
  QLabel *expenseLabel_{nullptr};
  QLabel *netLabel_{nullptr};
  QLabel *savingsLabel_{nullptr};
  QDateEdit *dashboardMonth_{nullptr};
  QTableWidget *transactionsTable_{nullptr};
  QTableWidget *accountsTable_{nullptr};
  QTableWidget *budgetsTable_{nullptr};
  QTableWidget *goalsTable_{nullptr};
  QTableWidget *recurringTable_{nullptr};
  QComboBox *transactionAccount_{nullptr};
  QComboBox *expenseAccount_{nullptr};
  QComboBox *incomeAccount_{nullptr};
  QComboBox *transferFrom_{nullptr};
  QComboBox *transferTo_{nullptr};
  QComboBox *budgetCategory_{nullptr};
  QComboBox *goalSelector_{nullptr};
  QComboBox *recurringAccount_{nullptr};
  QComboBox *recurringFrequency_{nullptr};
  QDateEdit *transactionDate_{nullptr};
  QLineEdit *transactionCategory_{nullptr};
  QLineEdit *transactionDescription_{nullptr};
  QDoubleSpinBox *transactionAmount_{nullptr};
  QDateEdit *forecastMonth_{nullptr};
  QSpinBox *forecastHistory_{nullptr};
  QLabel *forecastLabel_{nullptr};
  QLabel *analyticsLabel_{nullptr};
};
