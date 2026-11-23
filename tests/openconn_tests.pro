QT += core gui quick quickcontrols2 testlib

CONFIG += c++17 testcase
TARGET = openconn_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += \
    ../src/backend.h \
    ../src/theme.h

SOURCES += \
    openconn_tests.cpp \
    ../src/backend.cpp \
    ../src/theme.cpp
