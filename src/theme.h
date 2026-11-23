#pragma once

#include <QFileSystemWatcher>
#include <QObject>
#include <QString>

class Theme : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString accent READ accent NOTIFY changed)
    Q_PROPERTY(QString accentForeground READ accentForeground NOTIFY changed)
    Q_PROPERTY(int radius READ radius NOTIFY changed)

public:
    explicit Theme(const QString &baseDir = QString(), QObject *parent = nullptr);

    QString accent() const { return m_accent; }
    QString accentForeground() const { return m_accentForeground; }
    int radius() const { return m_radius; }

    static QString extractAccent(const QString &filePath);

signals:
    void changed();

private:
    void loadTheme();
    void updateRounding();

    QString m_baseDir;
    QString m_accent{"#2563EB"};
    QString m_accentForeground{"#FFFFFF"};
    int m_radius{8};
    QFileSystemWatcher m_watcher;
};
