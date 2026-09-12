#include "qt/MainWindow.hpp"
#include "core/Category.hpp"
#include "core/FinanceManager.hpp"
#include "services/CsvService.hpp"
#include "services/ReportService.hpp"
#include <QApplication>
#include <QComboBox>
#include <QDate>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QStatusBar>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>
#include <exception>
#include <stdexcept>

namespace {

QDoubleSpinBox *moneySpin(QWidget *parent) {
  auto *spin = new QDoubleSpinBox(parent);
  spin->setRange(0.01, 1e12);
  spin->setDecimals(2);
  spin->setSingleStep(10.0);
  spin->setPrefix("€ ");
  return spin;
}

QDateEdit *dateEdit(QWidget *parent) {
  auto *edit = new QDateEdit(QDate::currentDate(), parent);
  edit->setCalendarPopup(true);
  edit->setDisplayFormat("yyyy-MM-dd");
  return edit;
}

QTableWidget *table(QWidget *parent, int columns) {
  auto *t = new QTableWidget(0, columns, parent);
  t->setSelectionBehavior(QAbstractItemView::SelectRows);
  t->setSelectionMode(QAbstractItemView::SingleSelection);
  t->setEditTriggers(QAbstractItemView::NoEditTriggers);
  t->setAlternatingRowColors(true);
  t->setShowGrid(false);
  t->setFocusPolicy(Qt::NoFocus);
  t->verticalHeader()->setVisible(false);
  t->verticalHeader()->setDefaultSectionSize(40);
  t->horizontalHeader()->setStretchLastSection(true);
  t->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  return t;
}

QLabel *metricCard(const QString &title, const QString &accentClass,
                   QWidget *parent) {
  auto *card = new QFrame(parent);
  card->setObjectName("metricCard");
  card->setProperty("accent", accentClass);
  auto *layout = new QVBoxLayout(card);
  layout->setContentsMargins(18, 16, 18, 16);
  layout->setSpacing(5);
  auto *titleLabel = new QLabel(title, card);
  titleLabel->setObjectName("metricTitle");
  auto *valueLabel = new QLabel("—", card);
  valueLabel->setObjectName("metricValue");
  valueLabel->setProperty("metricValue", true);
  layout->addWidget(titleLabel);
  layout->addWidget(valueLabel);
  return valueLabel;
}

QFrame *panel(const QString &title, QWidget *parent,
              QVBoxLayout **body = nullptr) {
  auto *frame = new QFrame(parent);
  frame->setObjectName("panel");
  auto *layout = new QVBoxLayout(frame);
  layout->setContentsMargins(18, 16, 18, 16);
  layout->setSpacing(12);
  auto *label = new QLabel(title, frame);
  label->setObjectName("panelTitle");
  layout->addWidget(label);
  if (body)
    *body = layout;
  return frame;
}

QPushButton *primaryButton(const QString &text, QWidget *parent) {
  auto *b = new QPushButton(text, parent);
  b->setObjectName("primaryButton");
  return b;
}

QPushButton *secondaryButton(const QString &text, QWidget *parent) {
  auto *b = new QPushButton(text, parent);
  b->setObjectName("secondaryButton");
  return b;
}

QString money(double value) { return QString("€ %1").arg(value, 0, 'f', 2); }

} // namespace

MainWindow::MainWindow(FinanceManager &manager, QWidget *parent)
    : QMainWindow(parent), manager_(manager) {
  setWindowTitle("Personal Finance Tracker");
  setMinimumSize(1100, 720);
  resize(1360, 860);
  applyTheme();
  buildUi();
  refreshAll();
}

void MainWindow::applyTheme() {
  qApp->setStyleSheet(R"qss(
        QWidget {
            font-family: "Segoe UI", "Noto Sans", sans-serif;
            font-size: 13px;
            color: #263238;
        }
        QMainWindow, QWidget#contentRoot { background: #f4f6f8; }
        QFrame#sidebar {
            background: #17212b;
            border: none;
        }
        QLabel#appTitle { color: white; font-size: 20px; font-weight: 700; }
        QLabel#appSubtitle { color: #9fb0bf; font-size: 11px; }
        QListWidget#navigation {
            background: transparent;
            border: none;
            outline: none;
            color: #b8c5d0;
        }
        QListWidget#navigation::item {
            padding: 12px 14px;
            margin: 2px 8px;
            border-radius: 7px;
        }
        QListWidget#navigation::item:hover { background: #233342; color: white; }
        QListWidget#navigation::item:selected { background: #2f80ed; color: white; font-weight: 600; }
        QFrame#topbar {
            background: white;
            border: none;
            border-bottom: 1px solid #e3e8ed;
        }
        QLabel#pageTitle { font-size: 21px; font-weight: 700; color: #17212b; }
        QLabel#pageSubtitle { color: #71808f; }
        QFrame#panel, QFrame#metricCard {
            background: white;
            border: 1px solid #e1e7ec;
            border-radius: 10px;
        }
        QFrame#metricCard[accent="income"] { border-left: 4px solid #27ae60; }
        QFrame#metricCard[accent="expense"] { border-left: 4px solid #eb5757; }
        QFrame#metricCard[accent="net"] { border-left: 4px solid #2f80ed; }
        QFrame#metricCard[accent="saving"] { border-left: 4px solid #9b51e0; }
        QLabel#metricTitle { color: #7a8793; font-size: 12px; font-weight: 600; }
        QLabel#metricValue { color: #17212b; font-size: 23px; font-weight: 700; }
        QLabel#panelTitle { font-size: 15px; font-weight: 700; color: #263238; }
        QGroupBox {
            background: white;
            border: 1px solid #e1e7ec;
            border-radius: 10px;
            margin-top: 12px;
            padding: 18px 12px 12px 12px;
            font-weight: 600;
        }
        QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 6px; color: #425466; }
        QLineEdit, QComboBox, QDateEdit, QDoubleSpinBox, QSpinBox {
            background: white;
            border: 1px solid #ccd5dd;
            border-radius: 6px;
            padding: 8px 9px;
            min-height: 18px;
        }
        QLineEdit:focus, QComboBox:focus, QDateEdit:focus, QDoubleSpinBox:focus, QSpinBox:focus {
            border: 1px solid #2f80ed;
        }
        QPushButton {
            min-height: 34px;
            padding: 0 15px;
            border-radius: 6px;
            border: 1px solid #ccd5dd;
            background: white;
            font-weight: 600;
        }
        QPushButton:hover { background: #f0f4f7; }
        QPushButton#primaryButton { background: #2f80ed; border-color: #2f80ed; color: white; }
        QPushButton#primaryButton:hover { background: #246fcf; }
        QPushButton#dangerButton { color: #c0392b; border-color: #edc5c0; }
        QPushButton#successButton { background: #27ae60; border-color: #27ae60; color: white; }
        QTableWidget {
            background: white;
            border: 1px solid #e1e7ec;
            border-radius: 8px;
            alternate-background-color: #f8fafb;
            selection-background-color: #e9f2ff;
            selection-color: #17212b;
        }
        QHeaderView::section {
            background: #f7f9fb;
            color: #647382;
            border: none;
            border-bottom: 1px solid #e1e7ec;
            padding: 10px;
            font-weight: 700;
        }
        QTabWidget::pane { border: none; }
        QStatusBar { background: white; border-top: 1px solid #e1e7ec; color: #6c7a88; }
        QProgressBar { border: none; background: #e8edf1; border-radius: 5px; height: 9px; text-align: center; }
        QProgressBar::chunk { background: #2f80ed; border-radius: 5px; }
        QToolButton { border: none; padding: 7px; }
    )qss");
}

void MainWindow::buildUi() {
  auto *root = new QWidget(this);
  root->setObjectName("contentRoot");
  auto *rootLayout = new QHBoxLayout(root);
  rootLayout->setContentsMargins(0, 0, 0, 0);
  rootLayout->setSpacing(0);

  auto *sidebar = new QFrame(root);
  sidebar->setObjectName("sidebar");
  sidebar->setFixedWidth(235);
  auto *sideLayout = new QVBoxLayout(sidebar);
  sideLayout->setContentsMargins(8, 24, 8, 18);
  sideLayout->setSpacing(8);

  auto *title = new QLabel("Personal Finance", sidebar);
  title->setObjectName("appTitle");
  title->setContentsMargins(14, 0, 0, 0);
  auto *subtitle = new QLabel("Finance Tracker  •  v4.1", sidebar);
  subtitle->setObjectName("appSubtitle");
  subtitle->setContentsMargins(14, 0, 0, 12);
  sideLayout->addWidget(title);
  sideLayout->addWidget(subtitle);

  navigation_ = new QListWidget(sidebar);
  navigation_->setObjectName("navigation");
  navigation_->addItems({"▣   Dashboard", "↔   Transactions", "▤   Accounts",
                         "◈   Budgets", "◎   Savings goals", "↻   Recurring",
                         "◫   Analytics & forecast"});
  navigation_->setCurrentRow(0);
  sideLayout->addWidget(navigation_, 1);

  auto *help = new QLabel("SQLite database\nLocal & offline", sidebar);
  help->setObjectName("appSubtitle");
  help->setContentsMargins(14, 0, 0, 0);
  sideLayout->addWidget(help);

  auto *content = new QWidget(root);
  auto *contentLayout = new QVBoxLayout(content);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);

  auto *topbar = new QFrame(content);
  topbar->setObjectName("topbar");
  topbar->setFixedHeight(76);
  auto *topLayout = new QHBoxLayout(topbar);
  topLayout->setContentsMargins(26, 12, 26, 12);
  auto *pageTitle = new QLabel("Dashboard", topbar);
  pageTitle->setObjectName("pageTitle");
  auto *pageSubtitle =
      new QLabel("A clear view of your financial position", topbar);
  pageSubtitle->setObjectName("pageSubtitle");
  auto *titleBox = new QVBoxLayout;
  titleBox->setSpacing(1);
  titleBox->addWidget(pageTitle);
  titleBox->addWidget(pageSubtitle);
  topLayout->addLayout(titleBox);
  topLayout->addStretch();
  auto *refresh = secondaryButton("↻  Refresh", topbar);
  topLayout->addWidget(refresh);

  tabs_ = new QTabWidget(content);
  tabs_->tabBar()->hide();
  contentLayout->addWidget(topbar);
  contentLayout->addWidget(tabs_, 1);

  rootLayout->addWidget(sidebar);
  rootLayout->addWidget(content, 1);
  setCentralWidget(root);

  statusLabel_ = new QLabel("Ready", this);
  statusBar()->addPermanentWidget(statusLabel_, 1);
  statusBar()->showMessage("Local database connected", 3000);

  buildDashboardTab();
  buildTransactionsTab();
  buildAccountsTab();
  buildBudgetsTab();
  buildGoalsTab();
  buildRecurringTab();
  buildAnalyticsTab();

  connect(navigation_, &QListWidget::currentRowChanged, tabs_,
          &QTabWidget::setCurrentIndex);
  connect(navigation_, &QListWidget::currentRowChanged, this,
          [pageTitle, pageSubtitle](int index) {
            const QStringList titles = {"Dashboard",
                                        "Transactions",
                                        "Accounts",
                                        "Budgets",
                                        "Savings goals",
                                        "Recurring transactions",
                                        "Analytics & forecast"};
            const QStringList subtitles = {
                "A clear view of your financial position",
                "Review and record income and expenses",
                "Manage balances and move money between accounts",
                "Control monthly category spending",
                "Track progress toward your savings targets",
                "Automate regular financial activity",
                "Understand trends and plan ahead"};
            if (index >= 0 && index < titles.size()) {
              pageTitle->setText(titles[index]);
              pageSubtitle->setText(subtitles[index]);
            }
          });
  connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshAll);
}

void MainWindow::buildDashboardTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(18);

  auto *header = new QHBoxLayout;
  auto *period = new QLabel("Overview period", w);
  period->setStyleSheet("font-weight:600;color:#566574;");
  dashboardMonth_ = dateEdit(w);
  dashboardMonth_->setFixedWidth(140);
  auto *refresh = primaryButton("Refresh overview", w);
  header->addWidget(period);
  header->addWidget(dashboardMonth_);
  header->addWidget(refresh);
  header->addStretch();
  v->addLayout(header);

  auto *metrics = new QHBoxLayout;
  metrics->setSpacing(12);
  incomeLabel_ = metricCard("Income", "income", w);
  expenseLabel_ = metricCard("Expenses", "expense", w);
  netLabel_ = metricCard("Net cash flow", "net", w);
  savingsLabel_ = metricCard("Savings rate", "saving", w);
  metrics->addWidget(incomeLabel_->parentWidget());
  metrics->addWidget(expenseLabel_->parentWidget());
  metrics->addWidget(netLabel_->parentWidget());
  metrics->addWidget(savingsLabel_->parentWidget());
  v->addLayout(metrics);

  auto *lower = new QHBoxLayout;
  lower->setSpacing(16);
  QVBoxLayout *accountsBody = nullptr;
  auto *accountsPanel = panel("Accounts snapshot", w, &accountsBody);
  dashboardAccounts_ = new QLabel(accountsPanel);
  dashboardAccounts_->setWordWrap(true);
  dashboardAccounts_->setTextFormat(Qt::RichText);
  accountsBody->addWidget(dashboardAccounts_);
  accountsBody->addStretch();

  QVBoxLayout *goalsBody = nullptr;
  auto *goalsPanel = panel("Savings goals", w, &goalsBody);
  dashboardGoals_ = new QLabel(goalsPanel);
  dashboardGoals_->setWordWrap(true);
  dashboardGoals_->setTextFormat(Qt::RichText);
  goalsBody->addWidget(dashboardGoals_);
  goalsBody->addStretch();
  lower->addWidget(accountsPanel, 1);
  lower->addWidget(goalsPanel, 1);
  v->addLayout(lower);

  QVBoxLayout *summaryBody = nullptr;
  auto *summaryPanel = panel("Monthly summary", w, &summaryBody);
  dashboardSummary_ = new QLabel(summaryPanel);
  dashboardSummary_->setWordWrap(true);
  dashboardSummary_->setTextFormat(Qt::RichText);
  summaryBody->addWidget(dashboardSummary_);
  v->addWidget(summaryPanel);
  v->addStretch();

  connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshDashboard);
  connect(dashboardMonth_, &QDateEdit::dateChanged, this,
          &MainWindow::refreshDashboard);
  tabs_->addTab(w, "Dashboard");
}

void MainWindow::buildTransactionsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(16);

  auto *box = new QGroupBox("Record transaction", w);
  auto *f = new QFormLayout(box);
  f->setContentsMargins(18, 22, 18, 18);
  f->setHorizontalSpacing(22);
  transactionAccount_ = new QComboBox(box);
  transactionAmount_ = moneySpin(box);
  transactionCategory_ = new QLineEdit(box);
  transactionCategory_->setPlaceholderText("e.g. Food");
  transactionDescription_ = new QLineEdit(box);
  transactionDescription_->setPlaceholderText("Optional description");
  transactionDate_ = dateEdit(box);
  f->addRow("Account", transactionAccount_);
  f->addRow("Amount", transactionAmount_);
  f->addRow("Category", transactionCategory_);
  f->addRow("Description", transactionDescription_);
  f->addRow("Date", transactionDate_);
  auto *buttons = new QHBoxLayout;
  auto *in = new QPushButton("＋  Add income", box);
  in->setObjectName("successButton");
  auto *ex = new QPushButton("－  Add expense", box);
  ex->setObjectName("primaryButton");
  buttons->addWidget(in);
  buttons->addWidget(ex);
  buttons->addStretch();
  f->addRow("", buttons);
  v->addWidget(box);

  transactionsTable_ = table(w, 7);
  transactionsTable_->setHorizontalHeaderLabels(
      {"ID", "Account", "Date", "Type", "Category", "Amount", "Description"});
  transactionsTable_->setColumnWidth(0, 60);
  transactionsTable_->setColumnWidth(1, 100);
  transactionsTable_->setColumnWidth(2, 105);
  transactionsTable_->setColumnWidth(3, 90);
  transactionsTable_->setColumnWidth(4, 120);
  transactionsTable_->setColumnWidth(5, 110);
  v->addWidget(transactionsTable_, 1);

  auto *actions = new QHBoxLayout;
  auto *refresh = secondaryButton("Refresh", w);
  auto *exportBtn = secondaryButton("Export CSV", w);
  auto *importBtn = secondaryButton("Import CSV", w);
  actions->addWidget(refresh);
  actions->addStretch();
  actions->addWidget(importBtn);
  actions->addWidget(exportBtn);
  v->addLayout(actions);

  connect(in, &QPushButton::clicked, this, &MainWindow::addIncome);
  connect(ex, &QPushButton::clicked, this, &MainWindow::addExpense);
  connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshAll);
  connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportCsv);
  connect(importBtn, &QPushButton::clicked, this, &MainWindow::importCsv);
  tabs_->addTab(w, "Transactions");
}

void MainWindow::buildAccountsTab() {
  auto *w = new QWidget;
  auto *v = new QVBoxLayout(w);
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(16);

  auto *addPanel = new QGroupBox("Add account", w);
  auto *addLayout = new QHBoxLayout(addPanel);
  addLayout->setContentsMargins(18, 22, 18, 18);
  auto *name = new QLineEdit(addPanel);
  name->setPlaceholderText("e.g. Main checking, Savings, Cash");
  auto *add = primaryButton("Add account", addPanel);
  addLayout->addWidget(name, 1);
  addLayout->addWidget(add);
  v->addWidget(addPanel);

  accountsTable_ = table(w, 3);
  accountsTable_->setHorizontalHeaderLabels(
      {"ID", "Account", "Current balance"});
  accountsTable_->setColumnWidth(0, 70);
  accountsTable_->setColumnWidth(1, 300);
  v->addWidget(accountsTable_, 1);

  auto *transfer = new QGroupBox("Transfer funds", w);
  auto *f = new QFormLayout(transfer);
  f->setContentsMargins(18, 22, 18, 18);
  transferFrom_ = new QComboBox(transfer);
  transferTo_ = new QComboBox(transfer);
  auto *amount = moneySpin(transfer);
  auto *date = dateEdit(transfer);
  f->addRow("From account", transferFrom_);
  f->addRow("To account", transferTo_);
  f->addRow("Amount", amount);
  f->addRow("Date", date);
  auto *go = primaryButton("Transfer funds", transfer);
  f->addRow("", go);
  v->addWidget(transfer);

  connect(add, &QPushButton::clicked, this, [this, name] {
    try {
      if (name->text().trimmed().isEmpty())
        throw std::runtime_error("Account name is required.");
      manager_.addAccount(name->text().trimmed().toStdString());
      name->clear();
      statusBar()->showMessage("Account created", 3000);
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  });
  connect(go, &QPushButton::clicked, this, [this, amount, date] {
    try {
      manager_.transferFunds(transferFrom_->currentData().toInt(),
                             transferTo_->currentData().toInt(),
                             amount->value(),
                             date->date().toString("yyyy-MM-dd").toStdString());
      statusBar()->showMessage("Transfer completed", 3000);
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
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(16);

  auto *box = new QGroupBox("Monthly budget", w);
  auto *form = new QFormLayout(box);
  form->setContentsMargins(18, 22, 18, 18);
  budgetCategory_ = new QComboBox(box);
  for (const auto &c : Category::Defaults)
    budgetCategory_->addItem(QString::fromStdString(c));
  auto *limit = moneySpin(box);
  auto *add = primaryButton("Save budget", box);
  form->addRow("Category", budgetCategory_);
  form->addRow("Monthly limit", limit);
  form->addRow("", add);
  v->addWidget(box);

  budgetsTable_ = table(w, 4);
  budgetsTable_->setHorizontalHeaderLabels(
      {"ID", "Category", "Monthly limit", "Status"});
  v->addWidget(budgetsTable_, 1);

  connect(add, &QPushButton::clicked, this, [this, limit] {
    try {
      manager_.setBudget(budgetCategory_->currentText().toStdString(),
                         limit->value());
      statusBar()->showMessage("Budget saved", 3000);
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
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(16);

  auto *box = new QGroupBox("Create savings goal", w);
  auto *form = new QFormLayout(box);
  form->setContentsMargins(18, 22, 18, 18);
  auto *name = new QLineEdit(box);
  name->setPlaceholderText("e.g. Emergency fund");
  auto *target = moneySpin(box);
  auto *deadline = dateEdit(box);
  auto *add = primaryButton("Create goal", box);
  form->addRow("Goal name", name);
  form->addRow("Target", target);
  form->addRow("Deadline", deadline);
  form->addRow("", add);
  v->addWidget(box);

  goalsTable_ = table(w, 6);
  goalsTable_->setHorizontalHeaderLabels(
      {"ID", "Goal", "Saved", "Target", "Progress", "Deadline"});
  v->addWidget(goalsTable_, 1);

  auto *actions = new QHBoxLayout;
  auto *update = secondaryButton("Update selected", w);
  auto *del = secondaryButton("Delete selected", w);
  del->setObjectName("dangerButton");
  actions->addWidget(update);
  actions->addWidget(del);
  actions->addStretch();
  v->addLayout(actions);

  connect(add, &QPushButton::clicked, this, [this, name, target, deadline] {
    try {
      manager_.addGoal(name->text().trimmed().toStdString(), target->value(),
                       deadline->date().toString("yyyy-MM-dd").toStdString());
      name->clear();
      statusBar()->showMessage("Savings goal created", 3000);
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
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(16);

  auto *box = new QGroupBox("Create recurring expense", w);
  auto *f = new QFormLayout(box);
  f->setContentsMargins(18, 22, 18, 18);
  recurringAccount_ = new QComboBox(box);
  auto *amount = moneySpin(box);
  auto *cat = new QComboBox(box);
  for (const auto &c : Category::Defaults)
    cat->addItem(QString::fromStdString(c));
  auto *desc = new QLineEdit(box);
  desc->setPlaceholderText("e.g. Rent, streaming, insurance");
  recurringFrequency_ = new QComboBox(box);
  recurringFrequency_->addItems({"daily", "weekly", "monthly"});
  auto *next = dateEdit(box);
  f->addRow("Account", recurringAccount_);
  f->addRow("Amount", amount);
  f->addRow("Category", cat);
  f->addRow("Description", desc);
  f->addRow("Frequency", recurringFrequency_);
  f->addRow("Next date", next);
  auto *add = primaryButton("Add recurring expense", box);
  f->addRow("", add);
  v->addWidget(box);

  recurringTable_ = table(w, 8);
  recurringTable_->setHorizontalHeaderLabels({"ID", "Account", "Type", "Amount",
                                              "Category", "Frequency",
                                              "Next date", "Status"});
  v->addWidget(recurringTable_, 1);

  auto *actions = new QHBoxLayout;
  auto *toggle = secondaryButton("Pause / resume", w);
  auto *process = primaryButton("Process due transactions", w);
  actions->addWidget(toggle);
  actions->addWidget(process);
  actions->addStretch();
  v->addLayout(actions);

  connect(add, &QPushButton::clicked, this, [this, amount, cat, desc, next] {
    try {
      manager_.addRecurringTransaction(
          recurringAccount_->currentData().toInt(), amount->value(),
          cat->currentText().toStdString(), desc->text().toStdString(),
          TransactionType::Expense,
          recurringFrequency_->currentText().toStdString(),
          next->date().toString("yyyy-MM-dd").toStdString());
      statusBar()->showMessage("Recurring transaction created", 3000);
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
  v->setContentsMargins(26, 24, 26, 26);
  v->setSpacing(16);

  auto *forecastBox = new QGroupBox("Cash-flow forecast", w);
  auto *f = new QFormLayout(forecastBox);
  f->setContentsMargins(18, 22, 18, 18);
  forecastMonth_ = dateEdit(forecastBox);
  forecastHistory_ = new QSpinBox(forecastBox);
  forecastHistory_->setRange(1, 24);
  forecastHistory_->setValue(3);
  auto *go = primaryButton("Calculate forecast", forecastBox);
  forecastLabel_ = new QLabel(forecastBox);
  forecastLabel_->setWordWrap(true);
  forecastLabel_->setMinimumHeight(70);
  f->addRow("Target month", forecastMonth_);
  f->addRow("History months", forecastHistory_);
  f->addRow("", go);
  f->addRow("Result", forecastLabel_);
  v->addWidget(forecastBox);

  QVBoxLayout *analyticsBody = nullptr;
  auto *analyticsPanel = panel("Spending analytics", w, &analyticsBody);
  analyticsLabel_ = new QLabel(analyticsPanel);
  analyticsLabel_->setWordWrap(true);
  analyticsLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  analyticsLabel_->setStyleSheet("font-family:monospace; color:#455a64;");
  analyticsBody->addWidget(analyticsLabel_);
  v->addWidget(analyticsPanel, 1);

  auto *actions = new QHBoxLayout;
  auto *chart = primaryButton("Generate SVG chart", w);
  auto *refresh = secondaryButton("Refresh analytics", w);
  actions->addWidget(refresh);
  actions->addStretch();
  actions->addWidget(chart);
  v->addLayout(actions);

  connect(go, &QPushButton::clicked, this, &MainWindow::refreshForecast);
  connect(chart, &QPushButton::clicked, this, &MainWindow::generateChart);
  connect(refresh, &QPushButton::clicked, this, &MainWindow::refreshAll);
  tabs_->addTab(w, "Analytics");
}

QString MainWindow::currentMonth() const {
  return dashboardMonth_->date().toString("yyyy-MM");
}

void MainWindow::loadAccounts(QComboBox *combo, bool includeAll) {
  if (!combo)
    return;
  const int old = combo->currentData().toInt();
  combo->clear();
  if (includeAll)
    combo->addItem("All accounts", 0);
  for (const auto &account : manager_.listAccounts()) {
    combo->addItem(QString::fromStdString(account.name), account.id);
  }
  const int index = combo->findData(old);
  if (index >= 0)
    combo->setCurrentIndex(index);
}

void MainWindow::refreshDashboard() {
  const auto month = currentMonth().toStdString();
  const double income = manager_.monthlyIncome(month);
  const double expenses = manager_.monthlyExpenses(month);
  const double net = income - expenses;
  const double savings = income > 0 ? net * 100.0 / income : 0.0;

  incomeLabel_->setText(money(income));
  expenseLabel_->setText(money(expenses));
  netLabel_->setText(money(net));
  savingsLabel_->setText(QString("%1%").arg(savings, 0, 'f', 1));

  QString summary =
      QString("<b>%1</b><br>Income <b>%2</b> &nbsp; • &nbsp; Expenses "
              "<b>%3</b> &nbsp; • &nbsp; Net <b>%4</b><br><span "
              "style='color:#71808f'>Savings rate is based on income for the "
              "selected month.</span>")
          .arg(QString::fromStdString(month))
          .arg(money(income))
          .arg(money(expenses))
          .arg(money(net));
  dashboardSummary_->setText(summary);

  QString accountsHtml;
  double totalBalance = 0;
  for (const auto &account : manager_.listAccounts()) {
    totalBalance += account.balance;
    accountsHtml += QString("<div style='padding:6px 0'><b>%1</b><span "
                            "style='float:right'>%2</span></div>")
                        .arg(QString::fromStdString(account.name))
                        .arg(money(account.balance));
  }
  accountsHtml +=
      QString("<hr style='border:0;border-top:1px solid #e5eaee'><b>Total "
              "balance</b><span style='float:right'><b>%1</b></span>")
          .arg(money(totalBalance));
  dashboardAccounts_->setText(accountsHtml);

  QString goalsHtml;
  const auto goals = manager_.listGoals();
  if (goals.empty()) {
    goalsHtml = "<span style='color:#71808f'>No savings goals yet.</span>";
  } else {
    for (const auto &goal : goals) {
      const double progress =
          goal.targetAmount > 0
              ? std::min(100.0, goal.currentAmount * 100.0 / goal.targetAmount)
              : 0;
      goalsHtml +=
          QString("<div style='padding:5px 0'><b>%1</b> — %2 / %3 (%4%)</div>")
              .arg(QString::fromStdString(goal.name))
              .arg(money(goal.currentAmount))
              .arg(money(goal.targetAmount))
              .arg(progress, 0, 'f', 0);
    }
  }
  dashboardGoals_->setText(goalsHtml);

  statusLabel_->setText(
      QString("%1 accounts  •  %2 transactions  •  Database ready")
          .arg(manager_.listAccounts().size())
          .arg(manager_.listTransactions().size()));
}

void MainWindow::refreshAll() {
  loadAccounts(transactionAccount_);
  loadAccounts(transferFrom_);
  loadAccounts(transferTo_);
  loadAccounts(recurringAccount_);

  const auto accounts = manager_.listAccounts();
  if (accountsTable_) {
    accountsTable_->setRowCount(0);
    for (const auto &a : accounts) {
      const int row = accountsTable_->rowCount();
      accountsTable_->insertRow(row);
      accountsTable_->setItem(row, 0,
                              new QTableWidgetItem(QString::number(a.id)));
      accountsTable_->setItem(
          row, 1, new QTableWidgetItem(QString::fromStdString(a.name)));
      accountsTable_->setItem(row, 2, new QTableWidgetItem(money(a.balance)));
    }
  }

  if (transactionsTable_) {
    transactionsTable_->setRowCount(0);
    const auto transactions = manager_.listTransactions();
    for (const auto &t : transactions) {
      const int row = transactionsTable_->rowCount();
      transactionsTable_->insertRow(row);
      QStringList values = {QString::number(t.id()),
                            QString::number(t.accountId()),
                            QString::fromStdString(t.date()),
                            QString::fromStdString(toString(t.type())),
                            QString::fromStdString(t.category()),
                            money(t.amount()),
                            QString::fromStdString(t.description())};
      for (int c = 0; c < values.size(); ++c)
        transactionsTable_->setItem(row, c, new QTableWidgetItem(values[c]));
    }
  }

  if (budgetsTable_) {
    budgetsTable_->setRowCount(0);
    const auto budgets = manager_.listBudgets();
    const auto month = currentMonth().toStdString();
    for (const auto &b : budgets) {
      const int row = budgetsTable_->rowCount();
      budgetsTable_->insertRow(row);
      const double spent = manager_.categorySpent(month, b.category);
      const QString state =
          spent > b.monthlyLimit
              ? "Over budget"
              : (spent > b.monthlyLimit * 0.8 ? "Near limit" : "On track");
      budgetsTable_->setItem(row, 0,
                             new QTableWidgetItem(QString::number(b.id)));
      budgetsTable_->setItem(
          row, 1, new QTableWidgetItem(QString::fromStdString(b.category)));
      budgetsTable_->setItem(row, 2,
                             new QTableWidgetItem(money(b.monthlyLimit)));
      budgetsTable_->setItem(row, 3, new QTableWidgetItem(state));
    }
  }

  if (goalsTable_) {
    goalsTable_->setRowCount(0);
    for (const auto &g : manager_.listGoals()) {
      const int row = goalsTable_->rowCount();
      goalsTable_->insertRow(row);
      const double progress =
          g.targetAmount > 0
              ? std::min(100.0, g.currentAmount * 100.0 / g.targetAmount)
              : 0;
      goalsTable_->setItem(row, 0, new QTableWidgetItem(QString::number(g.id)));
      goalsTable_->setItem(
          row, 1, new QTableWidgetItem(QString::fromStdString(g.name)));
      goalsTable_->setItem(row, 2,
                           new QTableWidgetItem(money(g.currentAmount)));
      goalsTable_->setItem(row, 3, new QTableWidgetItem(money(g.targetAmount)));
      goalsTable_->setItem(
          row, 4,
          new QTableWidgetItem(QString("%1%").arg(progress, 0, 'f', 0)));
      goalsTable_->setItem(
          row, 5, new QTableWidgetItem(QString::fromStdString(g.deadline)));
    }
  }

  if (recurringTable_) {
    recurringTable_->setRowCount(0);
    for (const auto &r : manager_.listRecurringTransactions()) {
      const int row = recurringTable_->rowCount();
      recurringTable_->insertRow(row);
      QStringList values = {QString::number(r.id),
                            QString::number(r.accountId),
                            QString::fromStdString(toString(r.type)),
                            money(r.amount),
                            QString::fromStdString(r.category),
                            QString::fromStdString(r.frequency),
                            QString::fromStdString(r.nextDate),
                            r.active ? "Active" : "Paused"};
      for (int c = 0; c < values.size(); ++c)
        recurringTable_->setItem(row, c, new QTableWidgetItem(values[c]));
    }
  }

  refreshDashboard();
  if (analyticsLabel_)
    analyticsLabel_->setText(QString::fromStdString(
        manager_.financialAnalytics(currentMonth().toStdString())));
}

void MainWindow::addIncome() {
  try {
    manager_.addTransaction(
        transactionAccount_->currentData().toInt(), transactionAmount_->value(),
        transactionCategory_->text().trimmed().toStdString(),
        transactionDescription_->text().toStdString(),
        transactionDate_->date().toString("yyyy-MM-dd").toStdString(),
        TransactionType::Income);
    statusBar()->showMessage("Income transaction added", 3000);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::addExpense() {
  try {
    manager_.addTransaction(
        transactionAccount_->currentData().toInt(), transactionAmount_->value(),
        transactionCategory_->text().trimmed().toStdString(),
        transactionDescription_->text().toStdString(),
        transactionDate_->date().toString("yyyy-MM-dd").toStdString(),
        TransactionType::Expense);
    statusBar()->showMessage("Expense transaction added", 3000);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::addAccount() {}
void MainWindow::transferFunds() {}
void MainWindow::addBudget() {}
void MainWindow::addGoal() {}
void MainWindow::addRecurring() {}

void MainWindow::exportCsv() {
  const QString path = QFileDialog::getSaveFileName(
      this, "Export transactions", "transactions.csv", "CSV files (*.csv)");
  if (path.isEmpty())
    return;
  try {
    CsvService::exportTransactions(path.toStdString(),
                                   manager_.listTransactions());
    statusBar()->showMessage("Transactions exported", 3000);
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::importCsv() {
  const QString path = QFileDialog::getOpenFileName(
      this, "Import transactions", QString(), "CSV files (*.csv)");
  if (path.isEmpty())
    return;
  bool ok = false;
  const int accountId = QInputDialog::getInt(
      this, "Target account", "Account ID:", 1, 1, 100000, 1, &ok);
  if (!ok)
    return;
  try {
    for (const auto &t : CsvService::importTransactions(path.toStdString())) {
      manager_.addTransaction(accountId, t.amount(), t.category(),
                              t.description(), t.date(), t.type());
    }
    statusBar()->showMessage("Transactions imported", 3000);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

int MainWindow::selectedId(QTableWidget *t) const {
  if (!t || t->currentRow() < 0 || !t->item(t->currentRow(), 0))
    return 0;
  return t->item(t->currentRow(), 0)->text().toInt();
}

void MainWindow::updateGoal() {
  const int id = selectedId(goalsTable_);
  if (!id) {
    showError("Select a savings goal first.");
    return;
  }
  bool ok = false;
  const double value = QInputDialog::getDouble(
      this, "Update goal", "Current amount:", 0, 0, 1e12, 2, &ok);
  if (!ok)
    return;
  try {
    manager_.updateGoal(id, value);
    statusBar()->showMessage("Goal updated", 3000);
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::deleteGoal() {
  const int id = selectedId(goalsTable_);
  if (!id) {
    showError("Select a savings goal first.");
    return;
  }
  if (QMessageBox::question(this, "Delete goal",
                            "Delete the selected savings goal?") ==
      QMessageBox::Yes) {
    try {
      manager_.deleteGoal(id);
      statusBar()->showMessage("Goal deleted", 3000);
      refreshAll();
    } catch (const std::exception &e) {
      showError(e.what());
    }
  }
}

void MainWindow::processRecurring() {
  try {
    const int count = manager_.processRecurringTransactions(
        QDate::currentDate().toString("yyyy-MM-dd").toStdString());
    QMessageBox::information(
        this, "Recurring transactions",
        QString("%1 transaction(s) were created.").arg(count));
    refreshAll();
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::toggleRecurring() {
  const int id = selectedId(recurringTable_);
  if (!id) {
    showError("Select a recurring transaction first.");
    return;
  }
  for (const auto &recurring : manager_.listRecurringTransactions()) {
    if (recurring.id == id) {
      try {
        manager_.setRecurringTransactionActive(id, !recurring.active);
        statusBar()->showMessage("Recurring transaction updated", 3000);
        refreshAll();
      } catch (const std::exception &e) {
        showError(e.what());
      }
      return;
    }
  }
}

void MainWindow::refreshForecast() {
  try {
    const auto month = forecastMonth_->date().toString("yyyy-MM").toStdString();
    const int history = forecastHistory_->value();
    forecastLabel_->setText(QString::fromStdString(
        ReportService::forecastReport(manager_, month, history)));
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::generateChart() {
  const QString path = QFileDialog::getSaveFileName(
      this, "Save SVG chart", "finance-chart.svg", "SVG files (*.svg)");
  if (path.isEmpty())
    return;
  try {
    ReportService::generateMonthlyChart(manager_, currentMonth().toStdString(),
                                        path.toStdString());
    QMessageBox::information(this, "Chart generated",
                             "The monthly SVG chart was written successfully.");
  } catch (const std::exception &e) {
    showError(e.what());
  }
}

void MainWindow::showError(const QString &message) {
  QMessageBox::critical(this, "Personal Finance Tracker", message);
}

void MainWindow::setupNavigation() {}
