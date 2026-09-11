#include "core/FinanceManager.hpp"
#include "qt/MainWindow.hpp"
#include <QApplication>
int main(int argc, char **argv) {
  QApplication app(argc, argv);
  QString db = (argc > 1 ? QString::fromLocal8Bit(argv[1])
                         : QStringLiteral("data/finance.db"));
  FinanceManager manager(db.toStdString());
  try {
    manager.initialize();
    MainWindow w(manager);
    w.show();
    return app.exec();
  } catch (const std::exception &e) {
    return 1;
  }
}
