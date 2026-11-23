#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <cmath>

Theme::Theme(const QString &baseDir, QObject *parent)
    : QObject(parent),
      m_baseDir(baseDir.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : baseDir) {
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, &Theme::loadTheme);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, &Theme::loadTheme);

    loadTheme();
    updateRounding();
}

QString Theme::extractAccent(const QString &filePath) {
    QFile f(filePath);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QRegularExpression regex(R"re(accent\s*=\s*["'](#[0-9a-fA-F]{6})["'])re");
        while (!f.atEnd()) {
            QString line = QString::fromUtf8(f.readLine());
            auto match = regex.match(line);
            if (match.hasMatch()) {
                return match.captured(1);
            }
        }
    }
    return QString("#2563EB");
}

void Theme::loadTheme() {
    QString colorsPath = m_baseDir + "/theme/colors.toml";
    if (QFileInfo::exists(colorsPath)) {
        if (!m_watcher.files().contains(colorsPath)) {
            m_watcher.addPath(colorsPath);
        }
        m_accent = extractAccent(colorsPath);
    } else {
        m_accent = QString("#2563EB");
    }

    QColor color(m_accent);
    double r = color.redF() <= 0.03928 ? color.redF() / 12.92 : std::pow((color.redF() + 0.055) / 1.055, 2.4);
    double g = color.greenF() <= 0.03928 ? color.greenF() / 12.92 : std::pow((color.greenF() + 0.055) / 1.055, 2.4);
    double b = color.blueF() <= 0.03928 ? color.blueF() / 12.92 : std::pow((color.blueF() + 0.055) / 1.055, 2.4);
    double luminance = 0.2126 * r + 0.7152 * g + 0.0722 * b;
    m_accentForeground = luminance > 0.4 ? "#0F172A" : "#FFFFFF";

    emit changed();
}

void Theme::updateRounding() {
    QProcess hypr;
    hypr.start("hyprctl", {"-j", "getoption", "decoration:rounding"});
    if (hypr.waitForFinished(300)) {
        auto doc = QJsonDocument::fromJson(hypr.readAllStandardOutput());
        if (doc.isObject() && doc.object().contains("int")) {
            m_radius = doc.object().value("int").toInt(8);
            emit changed();
        }
    }
}
