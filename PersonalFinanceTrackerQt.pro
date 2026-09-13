QT += widgets charts
CONFIG += c++17
TEMPLATE = app
TARGET = personal_finance_tracker_qt
INCLUDEPATH += include
SOURCES += \
    src/qt/main_qt.cpp \
    src/qt/MainWindow.cpp \
    src/core/Transaction.cpp \
    src/core/Category.cpp \
    src/services/FinanceManager.cpp \
    src/services/CsvService.cpp \
    src/services/ReportService.cpp \
    src/storage/Database.cpp \
    src/utils/Date.cpp \
    src/utils/Format.cpp
LIBS += -lsqlite3
