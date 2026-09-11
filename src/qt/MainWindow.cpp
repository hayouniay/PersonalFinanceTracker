#include "qt/MainWindow.hpp"
#include "core/Category.hpp"
#include "core/FinanceManager.hpp"
#include "services/CsvService.hpp"
#include "services/ReportService.hpp"
#include "utils/Date.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDir>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextEdit>
#include <QVBoxLayout>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace {
QDoubleSpinBox *moneySpin(QWidget *p) {
  auto *s = new QDoubleSpinBox(p);
  s->setRange(0.01, 1e12);
  s->setDecimals(2);
  s->setSingleStep(10);
  return s;
}
QDateEdit *dateEdit(QWidget *p) {
  auto *d = new QDateEdit(QDate::currentDate(), p);
  d->setCalendarPopup(true);
  d->setDisplayFormat("yyyy-MM-dd");
  return d;
}
QTableWidget *table(QWidget *p, int cols) {
  auto *t = new QTableWidget(0, cols, p);
  t->setSelectionBehavior(QAbstractItemView::SelectRows);
  t->setSelectionMode(QAbstractItemView::SingleSelection);
  t->setEditTriggers(QAbstractItemView::NoEditTriggers);
  t->horizontalHeader()->setStretchLastSection(true);
  t->verticalHeader()->setVisible(false);
  return t;
}
QLabel *createMetricLabel(const QString &title, QWidget *p) {
  auto *l = new QLabel(title + "\n—", p);
  l->setAlignment(Qt::AlignCenter);
  l->setMinimumHeight(70);
  l->setStyleSheet("QLabel{border:1px solid "
                   "#bbb;border-radius:8px;padding:8px;font-size:15px;}");
  return l;
}
} // namespace

MainWindow::MainWindow(FinanceManager &manager, QWidget *parent)
    : QMainWindow(parent), manager_(manager) {
  setWindowTitle("Personal Finance Tracker — Qt5");
  resize(1180, 760);
  buildUi();
  refreshAll();
}

void MainWindow::buildUi() {
  tabs_ = new QTabWidget(this);
  setCentralWidget(tabs_);
  buildDashboardTab();
  buildTransactionsTab();
  buildAccountsTab();
  buildBudgetsTab();
  buildGoalsTab();
  buildRecurringTab();
  buildAnalyticsTab();
}

void MainWindow::buildDashboardTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *top = new QHBoxLayout;
  dashboardMonth_ = dateEdit(w);
  top->addWidget(new QLabel("Month:"));
  top->addWidget(dashboardMonth_);
  auto *b = new QPushButton("Refresh", w);
  connect(b, &QPushButton::clicked, this, &MainWindow::refreshDashboard);
  top->addWidget(b);
  top->addStretch();
  v->addLayout(top);
  auto *metrics = new QHBoxLayout;
  incomeLabel_ = createMetricLabel("Income", w);
  expenseLabel_ = createMetricLabel("Expenses", w);
  netLabel_ = createMetricLabel("Net", w);
  savingsLabel_ = createMetricLabel("Savings rate", w);
  metrics->addWidget(incomeLabel_);
  metrics->addWidget(expenseLabel_);
  metrics->addWidget(netLabel_);
  metrics->addWidget(savingsLabel_);
  v->addLayout(metrics);
  auto *hint = new QLabel(
      "Use the tabs to manage transactions, accounts, budgets, goals, "
      "recurring transactions and forecasts. The original console application "
      "remains available as a separate executable.",
      w);
  hint->setWordWrap(true);
  v->addWidget(hint);
  v->addStretch();
  tabs_->addTab(w, "Dashboard");
}

void MainWindow::buildTransactionsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *box = new QGroupBox("New transaction", w);
  auto *f = new QFormLayout(box);
  transactionAccount_ = new QComboBox(box);
  transactionAmount_ = moneySpin(box);
  transactionCategory_ = new QLineEdit(box);
  transactionDescription_ = new QLineEdit(box);
  transactionDate_ = dateEdit(box);
  f->addRow("Account", transactionAccount_);
  f->addRow("Amount", transactionAmount_);
  f->addRow("Category", transactionCategory_);
  f->addRow("Description", transactionDescription_);
  f->addRow("Date", transactionDate_);
  auto *buttons = new QHBoxLayout;
  auto *in = new QPushButton("Add income", box);
  auto *ex = new QPushButton("Add expense", box);
  buttons->addWidget(in);
  buttons->addWidget(ex);
  f->addRow(buttons);
  connect(in, &QPushButton::clicked, this, &MainWindow::addIncome);
  connect(ex, &QPushButton::clicked, this, &MainWindow::addExpense);
  v->addWidget(box);
  transactionsTable_ = table(w, 7);
  transactionsTable_->setHorizontalHeaderLabels(
      {"ID", "Account", "Date", "Type", "Category", "Amount", "Description"});
  v->addWidget(transactionsTable_);
  auto *actions = new QHBoxLayout;
  auto *refresh = new QPushButton("Refresh", w);
  auto *exportBtn = new QPushButton("Export CSV", w);
  auto *importBtn = new QPushButton("Import CSV", w);
  actions->addWidget(refresh);
  actions->addWidget(exportBtn);
  actions->addWidget(importBtn);
  actions->addStretch();
  v->addLayout(actions);
  connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshAll);
  connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportCsv);
  connect(importBtn, &QPushButton::clicked, this, &MainWindow::importCsv);
  tabs_->addTab(w, "Transactions");
}

void MainWindow::buildAccountsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *form = new QHBoxLayout;
  auto *name = new QLineEdit(w);
  name->setPlaceholderText("Account name");
  auto *add = new QPushButton("Add account", w);
  form->addWidget(name);
  form->addWidget(add);
  v->addLayout(form);
  accountsTable_ = table(w, 3);
  accountsTable_->setHorizontalHeaderLabels({"ID", "Name", "Balance"});
  v->addWidget(accountsTable_);
  auto *transfer = new QGroupBox("Transfer funds", w);
  auto *f = new QFormLayout(transfer);
  transferFrom_ = new QComboBox(transfer);
  transferTo_ = new QComboBox(transfer);
  auto *amt = moneySpin(transfer);
  auto *d = dateEdit(transfer);
  f->addRow("From", transferFrom_);
  f->addRow("To", transferTo_);
  f->addRow("Amount", amt);
  f->addRow("Date", d);
  auto *go = new QPushButton("Transfer", transfer);
  f->addRow(go);
  connect(go, &QPushButton::clicked, [this, amt, d]() {
    try {
      int from = transferFrom_->currentData().toInt(),
          to = transferTo_->currentData().toInt();
      manager_.transferFunds(from, to, amt->value(),
                             d->date().toString("yyyy-MM-dd").toStdString());
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  });
  v->addWidget(transfer);
  connect(add, &QPushButton::clicked, [this, name]() {
    try {
      manager_.addAccount(name->text().toStdString());
      name->clear();
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  });
  tabs_->addTab(w, "Accounts");
}

void MainWindow::buildBudgetsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *form = new QHBoxLayout;
  budgetCategory_ = new QComboBox(w);
  for (const auto &c : Category::Defaults)
    budgetCategory_->addItem(QString::fromStdString(c));
  auto *limit = moneySpin(w);
  auto *add = new QPushButton("Save budget", w);
  form->addWidget(new QLabel("Category"));
  form->addWidget(budgetCategory_);
  form->addWidget(new QLabel("Monthly limit"));
  form->addWidget(limit);
  form->addWidget(add);
  v->addLayout(form);
  budgetsTable_ = table(w, 3);
  budgetsTable_->setHorizontalHeaderLabels({"ID", "Category", "Monthly limit"});
  v->addWidget(budgetsTable_);
  connect(add, &QPushButton::clicked, [this, limit]() {
    try {
      manager_.setBudget(budgetCategory_->currentText().toStdString(),
                         limit->value());
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  });
  tabs_->addTab(w, "Budgets");
}

void MainWindow::buildGoalsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *form = new QHBoxLayout;
  auto *name = new QLineEdit(w);
  name->setPlaceholderText("Goal name");
  auto *target = moneySpin(w);
  auto *deadline = dateEdit(w);
  auto *add = new QPushButton("Add goal", w);
  form->addWidget(name);
  form->addWidget(new QLabel("Target"));
  form->addWidget(target);
  form->addWidget(new QLabel("Deadline"));
  form->addWidget(deadline);
  form->addWidget(add);
  v->addLayout(form);
  goalsTable_ = table(w, 5);
  goalsTable_->setHorizontalHeaderLabels(
      {"ID", "Name", "Current", "Target", "Deadline"});
  v->addWidget(goalsTable_);
  auto *actions = new QHBoxLayout;
  auto *update = new QPushButton("Update selected", w);
  auto *del = new QPushButton("Delete selected", w);
  actions->addWidget(update);
  actions->addWidget(del);
  actions->addStretch();
  v->addLayout(actions);
  connect(add, &QPushButton::clicked, [this, name, target, deadline]() {
    try {
      manager_.addGoal(name->text().toStdString(), target->value(),
                       deadline->date().toString("yyyy-MM-dd").toStdString());
      name->clear();
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  });
  connect(update, &QPushButton::clicked, this, &MainWindow::updateGoal);
  connect(del, &QPushButton::clicked, this, &MainWindow::deleteGoal);
  tabs_->addTab(w, "Savings goals");
}

void MainWindow::buildRecurringTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *box = new QGroupBox("New recurring transaction", w);
  auto *f = new QFormLayout(box);
  recurringAccount_ = new QComboBox(box);
  auto *amount = moneySpin(box);
  auto *cat = new QComboBox(box);
  for (const auto &c : Category::Defaults)
    cat->addItem(QString::fromStdString(c));
  auto *desc = new QLineEdit(box);
  recurringFrequency_ = new QComboBox(box);
  recurringFrequency_->addItems({"daily", "weekly", "monthly"});
  auto *d = dateEdit(box);
  f->addRow("Account", recurringAccount_);
  f->addRow("Amount", amount);
  f->addRow("Category", cat);
  f->addRow("Description", desc);
  f->addRow("Frequency", recurringFrequency_);
  f->addRow("Next date", d);
  auto *add = new QPushButton("Add recurring expense", box);
  f->addRow(add);
  v->addWidget(box);
  recurringTable_ = table(w, 8);
  recurringTable_->setHorizontalHeaderLabels({"ID", "Account", "Type", "Amount",
                                              "Category", "Frequency",
                                              "Next date", "Status"});
  v->addWidget(recurringTable_);
  auto *actions = new QHBoxLayout;
  auto *toggle = new QPushButton("Toggle selected", w);
  auto *process = new QPushButton("Process through today", w);
  actions->addWidget(toggle);
  actions->addWidget(process);
  actions->addStretch();
  v->addLayout(actions);
  connect(add, &QPushButton::clicked, [this, amount, cat, desc, d]() {
    try {
      manager_.addRecurringTransaction(
          recurringAccount_->currentData().toInt(), amount->value(),
          cat->currentText().toStdString(), desc->text().toStdString(),
          TransactionType::Expense,
          recurringFrequency_->currentText().toStdString(),
          d->date().toString("yyyy-MM-dd").toStdString());
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  });
  connect(toggle, &QPushButton::clicked, this, &MainWindow::toggleRecurring);
  connect(process, &QPushButton::clicked, this, &MainWindow::processRecurring);
  tabs_->addTab(w, "Recurring");
}

void MainWindow::buildAnalyticsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  auto *forecastBox = new QGroupBox("Forecast", w);
  auto *f = new QFormLayout(forecastBox);
  forecastMonth_ = dateEdit(forecastBox);
  forecastHistory_ = new QSpinBox(forecastBox);
  forecastHistory_->setRange(1, 24);
  forecastHistory_->setValue(3);
  forecastLabel_ = new QLabel(forecastBox);
  forecastLabel_->setWordWrap(true);
  auto *go = new QPushButton("Calculate forecast", forecastBox);
  f->addRow("Target month", forecastMonth_);
  f->addRow("History months", forecastHistory_);
  f->addRow(go);
  f->addRow(forecastLabel_);
  v->addWidget(forecastBox);
  analyticsLabel_ = new QLabel(w);
  analyticsLabel_->setWordWrap(true);
  analyticsLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  v->addWidget(analyticsLabel_);
  auto *chart = new QPushButton("Generate SVG chart", w);
  v->addWidget(chart);
  v->addStretch();
  connect(go, &QPushButton::clicked, this, &MainWindow::refreshForecast);
  connect(chart, &QPushButton::clicked, this, &MainWindow::generateChart);
  tabs_->addTab(w, "Analytics");
}

QString MainWindow::currentMonth() const {
  return dashboardMonth_->date().toString("yyyy-MM");
}
void MainWindow::loadAccounts(QComboBox *combo, bool includeAll) {
  if (!combo)
    return;
  int old = combo->currentData().toInt();
  combo->clear();
  if (includeAll)
    combo->addItem("All accounts", 0);
  for (const auto &a : manager_.listAccounts())
    combo->addItem(QString::fromStdString(a.name), a.id);
  int idx = combo->findData(old);
  if (idx >= 0)
    combo->setCurrentIndex(idx);
}
void MainWindow::refreshDashboard() {
  const auto m = currentMonth().toStdString();
  double in = manager_.monthlyIncome(m), ex = manager_.monthlyExpenses(m);
  incomeLabel_->setText("Income\n" + QString::number(in, 'f', 2));
  expenseLabel_->setText("Expenses\n" + QString::number(ex, 'f', 2));
  netLabel_->setText("Net\n" + QString::number(in - ex, 'f', 2));
  savingsLabel_->setText(
      "Savings rate\n" +
      QString::number(in ? ((in - ex) * 100.0 / in) : 0, 'f', 1) + "%");
}
void MainWindow::refreshAll() {
  loadAccounts(transactionAccount_);
  loadAccounts(transferFrom_);
  loadAccounts(transferTo_);
  loadAccounts(recurringAccount_);
  if (accountsTable_) {
    accountsTable_->setRowCount(0);
    for (const auto &a : manager_.listAccounts()) {
      int r = accountsTable_->rowCount();
      accountsTable_->insertRow(r);
      accountsTable_->setItem(r, 0,
                              new QTableWidgetItem(QString::number(a.id)));
      accountsTable_->setItem(
          r, 1, new QTableWidgetItem(QString::fromStdString(a.name)));
      accountsTable_->setItem(
          r, 2, new QTableWidgetItem(QString::number(a.balance, 'f', 2)));
    }
  }
  if (transactionsTable_) {
    transactionsTable_->setRowCount(0);
    for (const auto &t : manager_.listTransactions()) {
      int r = transactionsTable_->rowCount();
      transactionsTable_->insertRow(r);
      QStringList vals = {QString::number(t.id()),
                          QString::number(t.accountId()),
                          QString::fromStdString(t.date()),
                          QString::fromStdString(toString(t.type())),
                          QString::fromStdString(t.category()),
                          QString::number(t.amount(), 'f', 2),
                          QString::fromStdString(t.description())};
      for (int c = 0; c < vals.size(); ++c)
        transactionsTable_->setItem(r, c, new QTableWidgetItem(vals[c]));
    }
  }
  if (budgetsTable_) {
    budgetsTable_->setRowCount(0);
    for (const auto &b : manager_.listBudgets()) {
      int r = budgetsTable_->rowCount();
      budgetsTable_->insertRow(r);
      budgetsTable_->setItem(r, 0, new QTableWidgetItem(QString::number(b.id)));
      budgetsTable_->setItem(
          r, 1, new QTableWidgetItem(QString::fromStdString(b.category)));
      budgetsTable_->setItem(
          r, 2, new QTableWidgetItem(QString::number(b.monthlyLimit, 'f', 2)));
    }
  }
  if (goalsTable_) {
    goalsTable_->setRowCount(0);
    for (const auto &g : manager_.listGoals()) {
      int r = goalsTable_->rowCount();
      goalsTable_->insertRow(r);
      goalsTable_->setItem(r, 0, new QTableWidgetItem(QString::number(g.id)));
      goalsTable_->setItem(
          r, 1, new QTableWidgetItem(QString::fromStdString(g.name)));
      goalsTable_->setItem(
          r, 2, new QTableWidgetItem(QString::number(g.currentAmount, 'f', 2)));
      goalsTable_->setItem(
          r, 3, new QTableWidgetItem(QString::number(g.targetAmount, 'f', 2)));
      goalsTable_->setItem(
          r, 4, new QTableWidgetItem(QString::fromStdString(g.deadline)));
    }
  }
  if (recurringTable_) {
    recurringTable_->setRowCount(0);
    for (const auto &r : manager_.listRecurringTransactions()) {
      int row = recurringTable_->rowCount();
      recurringTable_->insertRow(row);
      QStringList vals = {QString::number(r.id),
                          QString::number(r.accountId),
                          QString::fromStdString(toString(r.type)),
                          QString::number(r.amount, 'f', 2),
                          QString::fromStdString(r.category),
                          QString::fromStdString(r.frequency),
                          QString::fromStdString(r.nextDate),
                          r.active ? "Active" : "Paused"};
      for (int c = 0; c < vals.size(); ++c)
        recurringTable_->setItem(row, c, new QTableWidgetItem(vals[c]));
    }
  }
  refreshDashboard();
  analyticsLabel_->setText(QString::fromStdString(
      manager_.financialAnalytics(currentMonth().toStdString())));
}

void MainWindow::addIncome() {
  try {
    manager_.addTransaction(
        transactionAccount_->currentData().toInt(), transactionAmount_->value(),
        transactionCategory_->text().toStdString(),
        transactionDescription_->text().toStdString(),
        transactionDate_->date().toString("yyyy-MM-dd").toStdString(),
        TransactionType::Income);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::addExpense() {
  try {
    manager_.addTransaction(
        transactionAccount_->currentData().toInt(), transactionAmount_->value(),
        transactionCategory_->text().toStdString(),
        transactionDescription_->text().toStdString(),
        transactionDate_->date().toString("yyyy-MM-dd").toStdString(),
        TransactionType::Expense);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::addAccount() {}
void MainWindow::transferFunds() {}
void MainWindow::exportCsv() {
  QString p = QFileDialog::getSaveFileName(
      this, "Export transactions", "transactions.csv", "CSV files (*.csv)");
  if (p.isEmpty())
    return;
  try {
    CsvService::exportTransactions(p.toStdString(),
                                   manager_.listTransactions());
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::importCsv() {
  QString p = QFileDialog::getOpenFileName(this, "Import transactions",
                                           QString(), "CSV files (*.csv)");
  if (p.isEmpty())
    return;
  bool ok = false;
  int aid = QInputDialog::getInt(this, "Target account", "Account ID:", 1, 1,
                                 100000, 1, &ok);
  if (!ok)
    return;
  try {
    for (const auto &t : CsvService::importTransactions(p.toStdString()))
      manager_.addTransaction(aid, t.amount(), t.category(), t.description(),
                              t.date(), t.type());
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::addBudget() {}
void MainWindow::addGoal() {}
int MainWindow::selectedId(QTableWidget *t) const {
  if (!t || t->currentRow() < 0)
    return 0;
  return t->item(t->currentRow(), 0)->text().toInt();
}
void MainWindow::updateGoal() {
  int id = selectedId(goalsTable_);
  if (!id) {
    showError("Select a goal first.");
    return;
  }
  bool ok = false;
  double v = QInputDialog::getDouble(this, "Update goal", "Current amount:", 0,
                                     0, 1e12, 2, &ok);
  if (!ok)
    return;
  try {
    manager_.updateGoal(id, v);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::deleteGoal() {
  int id = selectedId(goalsTable_);
  if (!id) {
    showError("Select a goal first.");
    return;
  }
  if (QMessageBox::question(this, "Delete goal", "Delete selected goal?") ==
      QMessageBox::Yes) {
    try {
      manager_.deleteGoal(id);
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  }
}
void MainWindow::addRecurring() {}
void MainWindow::processRecurring() {
  try {
    int n = manager_.processRecurringTransactions(
        QDate::currentDate().toString("yyyy-MM-dd").toStdString());
    QMessageBox::information(this, "Recurring",
                             QString::number(n) + " transaction(s) created.");
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::toggleRecurring() {
  int id = selectedId(recurringTable_);
  if (!id) {
    showError("Select a recurring transaction first.");
    return;
  }
  auto list = manager_.listRecurringTransactions();
  for (const auto &r : list)
    if (r.id == id) {
      try {
        manager_.setRecurringTransactionActive(id, !r.active);
        refreshAll();
      } catch (const std::exception &e) {
        showError(e.what());
      }
      return;
    }
}
void MainWindow::refreshForecast() {
  try {
    auto m = forecastMonth_->date().toString("yyyy-MM").toStdString();
    int h = forecastHistory_->value();
    forecastLabel_->setText(
        QString::fromStdString(ReportService::forecastReport(manager_, m, h)));
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::generateChart() {
  QString p = QFileDialog::getSaveFileName(
      this, "Save SVG chart", "finance-chart.svg", "SVG files (*.svg)");
  if (p.isEmpty())
    return;
  try {
    ReportService::generateMonthlyChart(manager_, currentMonth().toStdString(),
                                        p.toStdString());
    QMessageBox::information(this, "Chart", "Chart written to " + p);
  } catch (const std::exception &e) {
    showError(e.what());
  }
}
void MainWindow::showError(const QString &m) {
  QMessageBox::critical(this, "Personal Finance Tracker", m);
}
