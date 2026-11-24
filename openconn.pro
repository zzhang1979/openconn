QT += core gui qml quick quickcontrols2 network widgets

CONFIG += c++17 release
TARGET = openconn
TEMPLATE = app

HEADERS += \
    src/backend.h \
    src/theme.h

SOURCES += \
    src/main.cpp \
    src/backend.cpp \
    src/theme.cpp

RESOURCES += src/resources.qrc
