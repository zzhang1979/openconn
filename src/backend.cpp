#include "backend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QUuid>
#include <csignal>
#include <sys/types.h>

Backend::Backend(const QString &configDir, QObject *parent)
    : QObject(parent),
      m_configDir(configDir.isEmpty() ? QDir::homePath() + "/.config/openconn" : configDir) {
    m_configFile = m_configDir + "/profiles.json";

    m_durationTimer = new QTimer(this);
    m_durationTimer->setInterval(1000);
    connect(m_durationTimer, &QTimer::timeout, this, &Backend::onTimerTick);

    loadProfiles();
    checkPolkitRule();
}

Backend::~Backend() {
    if (m_process && m_process->state() != QProcess::NotRunning) {
        disconnectVpn();
        m_process->waitForFinished(2000);
    }
}

void Backend::checkPolkitRule() {
    bool found = QFile::exists("/etc/polkit-1/rules.d/50-openconnect.rules") ||
                 QFile::exists("/usr/share/polkit-1/rules.d/50-openconnect.rules");
    if (m_hasPolkitRule != found) {
        m_hasPolkitRule = found;
        emit hasPolkitRuleChanged();
    }
}

QString Backend::getPolkitRuleSnippet() const {
    return QString(
        "/* Place in /etc/polkit-1/rules.d/50-openconnect.rules */\n"
        "polkit.addRule(function(action, subject) {\n"
        "    if (action.id == \"org.freedesktop.policykit.exec\" &&\n"
        "        action.lookup(\"program\") == \"/usr/bin/openconnect\" &&\n"
        "        subject.isInGroup(\"wheel\")) {\n"
        "        return polkit.Result.YES;\n"
        "    }\n"
        "});\n"
    );
}

void Backend::loadProfiles() {
    QDir dir(m_configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(m_configFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        auto doc = QJsonDocument::fromJson(file.readAll());
        if (doc.isArray()) {
            m_profilesData = doc.array();
        }
    }

    if (m_profilesData.isEmpty()) {
        QJsonObject defProfile;
        defProfile["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
        defProfile["name"] = QString("My VPN");
        defProfile["server"] = QString("");
        defProfile["protocol"] = QString("anyconnect");
        defProfile["username"] = QString("");
        defProfile["password"] = QString("");
        defProfile["authgroup"] = QString("");
        defProfile["servercert"] = QString("");
        defProfile["allowSelfSigned"] = false;
        defProfile["extraArgs"] = QString("");
        m_profilesData.append(defProfile);
        saveProfiles();
    }

    m_activeIndex = 0;
    emit profilesChanged();
    emit activeIndexChanged();
    emit activeProfileChanged();
}

void Backend::saveProfiles() {
    QDir dir(m_configDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    QFile file(m_configFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QJsonDocument doc(m_profilesData);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        file.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    }
}

QVariantList Backend::profiles() const {
    return m_profilesData.toVariantList();
}

QVariantMap Backend::activeProfile() const {
    if (m_activeIndex >= 0 && m_activeIndex < m_profilesData.size()) {
        return m_profilesData.at(m_activeIndex).toObject().toVariantMap();
    }
    return QVariantMap();
}

void Backend::setActiveIndex(int index) {
    if (index >= 0 && index < m_profilesData.size() && index != m_activeIndex) {
        m_activeIndex = index;
        emit activeIndexChanged();
        emit activeProfileChanged();
    }
}

void Backend::addProfile(const QString &name) {
    QJsonObject profile;
    profile["id"] = QUuid::createUuid().toString(QUuid::WithoutBraces);
    profile["name"] = name.isEmpty() ? QString("VPN Profile %1").arg(m_profilesData.size() + 1) : name;
    profile["server"] = QString("");
    profile["protocol"] = QString("anyconnect");
    profile["username"] = QString("");
    profile["password"] = QString("");
    profile["authgroup"] = QString("");
    profile["servercert"] = QString("");
    profile["allowSelfSigned"] = false;
    profile["extraArgs"] = QString("");

    m_profilesData.append(profile);
    saveProfiles();
    m_activeIndex = m_profilesData.size() - 1;

    emit profilesChanged();
    emit activeIndexChanged();
    emit activeProfileChanged();
}

void Backend::removeProfile(int index) {
    if (index >= 0 && index < m_profilesData.size() && m_profilesData.size() > 1) {
        m_profilesData.removeAt(index);
        saveProfiles();
        if (m_activeIndex >= m_profilesData.size()) {
            m_activeIndex = m_profilesData.size() - 1;
        }
        emit profilesChanged();
        emit activeIndexChanged();
        emit activeProfileChanged();
    }
}

void Backend::updateProfile(int index, const QVariantMap &data) {
    if (index >= 0 && index < m_profilesData.size()) {
        QJsonObject obj = QJsonObject::fromVariantMap(data);
        m_profilesData[index] = obj;
        saveProfiles();
        emit profilesChanged();
        if (index == m_activeIndex) {
            emit activeProfileChanged();
        }
    }
}

void Backend::clearLogs() {
    m_logs.clear();
    emit logsChanged();
}

void Backend::appendLog(const QString &line) {
    if (line.trimmed().isEmpty()) return;
    m_logs += line + "\n";
    if (m_logs.size() > 100000) {
        m_logs = m_logs.right(80000);
    }
    emit logsChanged();
    parseLogOutput(line);
}

void Backend::parseLogOutput(const QString &text) {
    // Detect assigned IP
    static const QRegularExpression ipRegex(R"re(Connected as\s+([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+))re");
    auto matchIp = ipRegex.match(text);
    if (matchIp.hasMatch()) {
        m_assignedIp = matchIp.captured(1);
        emit assignedIpChanged();
    }

    // Detect interface
    static const QRegularExpression ifRegex(R"re(Configured (tun[0-9]+|tap[0-9]+|vpn[0-9]+))re");
    auto matchIf = ifRegex.match(text);
    if (matchIf.hasMatch()) {
        m_vpnInterface = matchIf.captured(1);
        emit vpnInterfaceChanged();
    }

    // Detect successful tunnel establishment
    if (text.contains("Established DTLS connection") ||
        text.contains("Connected as ") ||
        text.contains("Session established") ||
        (text.contains("POST https") && text.contains("Connected"))) {
        if (m_state != Connected) {
            setState(Connected, "Connected");
            m_connectedTimestamp = QDateTime::currentSecsSinceEpoch();
            m_durationTimer->start();
        }
    }
}

void Backend::setState(State newState, const QString &text) {
    m_state = newState;
    m_statusText = text.isEmpty() ? (newState == Connected ? "Connected" :
                                    newState == Connecting ? "Connecting..." :
                                    newState == Disconnecting ? "Disconnecting..." :
                                    newState == Error ? "Error" : "Disconnected")
                                  : text;
    emit connectionStateChanged();
    emit statusTextChanged();
}

void Backend::connectVpn() {
    if (m_state == Connected || m_state == Connecting) {
        return;
    }

    QVariantMap profile = activeProfile();
    QString server = profile.value("server").toString().trimmed();
    QString protocol = profile.value("protocol").toString().trimmed();
    QString username = profile.value("username").toString().trimmed();
    QString password = profile.value("password").toString();
    QString authgroup = profile.value("authgroup").toString().trimmed();
    QString servercert = profile.value("servercert").toString().trimmed();
    bool allowSelfSigned = profile.value("allowSelfSigned").toBool();
    QString extraArgs = profile.value("extraArgs").toString().trimmed();

    if (server.isEmpty()) {
        setState(Error, "Server URL is required");
        appendLog("[Error] Please configure a Server/Gateway address before connecting.");
        return;
    }

    setState(Connecting, "Authenticating...");
    m_assignedIp = "—";
    m_vpnInterface = "—";
    m_connectedTime = "00:00:00";
    emit assignedIpChanged();
    emit vpnInterfaceChanged();
    emit connectedTimeChanged();

    appendLog(QString("[OpenConn] Starting connection to %1 (%2)...").arg(server, protocol));

    if (m_process) {
        m_process->deleteLater();
        m_process = nullptr;
    }

    m_process = new QProcess(this);

    QStringList arguments;
    arguments << "/usr/bin/openconnect";
    arguments << server;
    if (!protocol.isEmpty()) {
        arguments << QString("--protocol=%1").arg(protocol);
    }
    if (!username.isEmpty()) {
        arguments << QString("--user=%1").arg(username);
    }
    arguments << "--passwd-on-stdin";
    if (!authgroup.isEmpty()) {
        arguments << QString("--authgroup=%1").arg(authgroup);
    }
    if (!servercert.isEmpty()) {
        arguments << QString("--servercert=%1").arg(servercert);
    }
    if (allowSelfSigned) {
        arguments << "--no-system-trust";
    }
    if (!extraArgs.isEmpty()) {
        arguments << extraArgs.split(" ", Qt::SkipEmptyParts);
    }

    connect(m_process, &QProcess::readyReadStandardOutput, this, &Backend::onProcessReadyRead);
    connect(m_process, &QProcess::readyReadStandardError, this, &Backend::onProcessReadyRead);
    connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Backend::onProcessFinished);
    connect(m_process, &QProcess::errorOccurred, this, &Backend::onProcessError);

    // Run openconnect elevated with pkexec
    m_process->start("pkexec", arguments);

    // Provide the saved password via stdin immediately
    if (m_process->waitForStarted(2000)) {
        if (!password.isEmpty()) {
            m_process->write(password.toUtf8() + "\n");
        }
    }
}

void Backend::disconnectVpn() {
    if (m_state == Disconnected) return;

    setState(Disconnecting, "Disconnecting...");
    m_durationTimer->stop();
    appendLog("[OpenConn] Sending termination signal to disconnect cleanly...");

    if (m_process && m_process->state() != QProcess::NotRunning) {
        // Send SIGINT so openconnect executes its teardown vpnc-script
        pid_t pid = m_process->processId();
        if (pid > 0) {
            ::kill(pid, SIGINT);
        }
        m_process->terminate();

        QTimer::singleShot(4000, this, [this]() {
            if (m_process && m_process->state() != QProcess::NotRunning) {
                appendLog("[OpenConn] Process did not terminate; forcing kill.");
                m_process->kill();
            }
        });
    } else {
        setState(Disconnected, "Disconnected");
    }
}

void Backend::toggleConnect() {
    if (m_state == Connected || m_state == Connecting) {
        disconnectVpn();
    } else {
        connectVpn();
    }
}

void Backend::onProcessReadyRead() {
    if (!m_process) return;

    QByteArray out = m_process->readAllStandardOutput();
    if (!out.isEmpty()) {
        QString text = QString::fromUtf8(out);
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            appendLog(line);
        }
    }

    QByteArray err = m_process->readAllStandardError();
    if (!err.isEmpty()) {
        QString text = QString::fromUtf8(err);
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            appendLog(line);
        }
    }
}

void Backend::onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    m_durationTimer->stop();
    appendLog(QString("[OpenConn] Connection process ended (exit code: %1).").arg(exitCode));

    if (m_state == Disconnecting || exitCode == 0) {
        setState(Disconnected, "Disconnected");
    } else {
        setState(Error, QString("Failed (code %1)").arg(exitCode));
    }
    m_assignedIp = "—";
    m_vpnInterface = "—";
    emit assignedIpChanged();
    emit vpnInterfaceChanged();
}

void Backend::onProcessError(QProcess::ProcessError error) {
    if (m_state == Disconnecting) return;

    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "Failed to launch pkexec/openconnect. Check if pkexec is available.";
        break;
    case QProcess::Crashed:
        errorMsg = "OpenConnect process crashed unexpectedly.";
        break;
    case QProcess::Timedout:
        errorMsg = "Connection timed out.";
        break;
    default:
        errorMsg = "An error occurred with the OpenConnect process.";
        break;
    }

    appendLog(QString("[OpenConn Error] %1").arg(errorMsg));
    setState(Error, errorMsg);
}

void Backend::onTimerTick() {
    if (m_state != Connected || m_connectedTimestamp <= 0) return;

    qint64 elapsed = QDateTime::currentSecsSinceEpoch() - m_connectedTimestamp;
    int hours = static_cast<int>(elapsed / 3600);
    int minutes = static_cast<int>((elapsed % 3600) / 60);
    int seconds = static_cast<int>(elapsed % 60);

    m_connectedTime = QString("%1:%2:%3")
                          .arg(hours, 2, 10, QChar('0'))
                          .arg(minutes, 2, 10, QChar('0'))
                          .arg(seconds, 2, 10, QChar('0'));
    emit connectedTimeChanged();
}
