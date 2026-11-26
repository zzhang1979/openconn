#include <QApplication>
#include <QIcon>
#include <QMenu>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSystemTrayIcon>
#include <QUrl>

#include "backend.h"
#include "theme.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("openconn");
    app.setApplicationDisplayName("OpenConn");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("openconn");

    for (int i = 1; i < argc; ++i) {
        QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == "--version" || arg == "-v") {
            fprintf(stdout, "OpenConn %s\n", app.applicationVersion().toUtf8().constData());
            return 0;
        }
    }

    QIcon appIcon(":/openconn.svg");
    if (appIcon.isNull()) {
        appIcon = QIcon::fromTheme("openconn");
    }
    app.setWindowIcon(appIcon);

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

    QQuickWindow *mainWindow = qobject_cast<QQuickWindow*>(engine.rootObjects().first());

    // Setup System Tray Icon
    QSystemTrayIcon trayIcon(appIcon, &app);
    trayIcon.setToolTip("OpenConn — VPN Client");

    QMenu trayMenu;
    QAction *showAction = trayMenu.addAction("Show OpenConn");
    QAction *toggleVpnAction = trayMenu.addAction("Connect VPN");
    trayMenu.addSeparator();
    QAction *quitAction = trayMenu.addAction("Quit");

    trayIcon.setContextMenu(&trayMenu);

    // Update tray status dynamically
    auto updateTray = [&]() {
        if (backend.connectionState() == Backend::Connected) {
            toggleVpnAction->setText("Disconnect VPN");
            trayIcon.setToolTip(QString("OpenConn: Connected (%1)").arg(backend.assignedIp()));
        } else if (backend.connectionState() == Backend::Connecting) {
            toggleVpnAction->setText("Connecting...");
            trayIcon.setToolTip("OpenConn: Connecting...");
        } else if (backend.connectionState() == Backend::Disconnecting) {
            toggleVpnAction->setText("Disconnecting...");
            trayIcon.setToolTip("OpenConn: Disconnecting...");
        } else {
            toggleVpnAction->setText("Connect VPN");
            trayIcon.setToolTip("OpenConn: Disconnected");
        }
    };

    QObject::connect(&backend, &Backend::connectionStateChanged, updateTray);
    QObject::connect(&backend, &Backend::assignedIpChanged, updateTray);

    QObject::connect(toggleVpnAction, &QAction::triggered, &backend, &Backend::toggleConnect);
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    auto showWindow = [mainWindow]() {
        if (mainWindow) {
            mainWindow->showNormal();
            mainWindow->raise();
            mainWindow->requestActivate();
        }
    };

    QObject::connect(showAction, &QAction::triggered, showWindow);

    QObject::connect(&trayIcon, &QSystemTrayIcon::activated, [mainWindow](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            if (mainWindow) {
                if (mainWindow->isVisible() && mainWindow->visibility() != QWindow::Minimized) {
                    mainWindow->hide();
                } else {
                    mainWindow->showNormal();
                    mainWindow->raise();
                    mainWindow->requestActivate();
                }
            }
        }
    });

    trayIcon.show();
    updateTray();

    int ret = app.exec();

    QObject::disconnect(&backend, nullptr, nullptr, nullptr);

    return ret;
}
