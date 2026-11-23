#include <QGuiApplication>
#include <QIcon>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>

#include "backend.h"
#include "theme.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("openconn");
    app.setApplicationDisplayName("OpenConn");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("openconn");
    app.setWindowIcon(QIcon::fromTheme("openconn"));

    QQuickStyle::setStyle("Material");

    Theme theme;
    Backend backend;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("theme", &theme);
    engine.rootContext()->setContextProperty("backend", &backend);

    engine.load(QUrl("qrc:/Main.qml"));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }

    return app.exec();
}
