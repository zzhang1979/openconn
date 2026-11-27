#include "backend.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QUuid>
#include <csignal>
#include <sys/types.h>

bool Backend::isOpenconnectInstalled() const {
    return QFile::exists("/usr/bin/openconnect") || !QStandardPaths::findExecutable("openconnect").isEmpty();
}

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
    disconnect();
    disconnectVpn();
    if (m_tunnelProcess && m_tunnelProcess->state() != QProcess::NotRunning) {
        m_tunnelProcess->waitForFinished(1000);
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
        "        var cmd = action.lookup(\"command_line\") || \"\";\n"
        "        // Prevent execution of arbitrary custom scripts or trojans as root\n"
        "        if (cmd.indexOf(\"--script=\") !== -1 || cmd.indexOf(\" -s \") !== -1 || cmd.indexOf(\" -s=\") !== -1) {\n"
        "            if (cmd.indexOf(\"--script=/etc/vpnc/vpnc-script\") === -1 &&\n"
        "                cmd.indexOf(\"-s /etc/vpnc/vpnc-script\") === -1) {\n"
        "                return polkit.Result.NO;\n"
        "            }\n"
        "        }\n"
        "        if (cmd.indexOf(\"--csd-wrapper\") !== -1 || cmd.indexOf(\"--trojan\") !== -1) {\n"
        "            return polkit.Result.NO;\n"
        "        }\n"
        "        return polkit.Result.YES;\n"
        "    }\n"
        "    if (action.id.indexOf(\"org.freedesktop.resolve1.\") === 0 &&\n"
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
    emitActiveProfileSignals();
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

QJsonObject Backend::currentProfileObject() const {
    if (m_activeIndex >= 0 && m_activeIndex < m_profilesData.size()) {
        return m_profilesData.at(m_activeIndex).toObject();
    }
    return QJsonObject();
}

void Backend::updateCurrentProfileField(const QString &key, const QJsonValue &value) {
    if (m_activeIndex >= 0 && m_activeIndex < m_profilesData.size()) {
        QJsonObject obj = m_profilesData.at(m_activeIndex).toObject();
        if (obj.value(key) != value) {
            obj[key] = value;
            m_profilesData[m_activeIndex] = obj;
            saveProfiles();
        }
    }
}

QVariantList Backend::profiles() const {
    return m_profilesData.toVariantList();
}

void Backend::setActiveIndex(int index) {
    if (index >= 0 && index < m_profilesData.size() && index != m_activeIndex) {
        m_activeIndex = index;
        emit activeIndexChanged();
        emitActiveProfileSignals();
    }
}

void Backend::emitActiveProfileSignals() {
    emit profileNameChanged();
    emit profileServerChanged();
    emit profileProtocolChanged();
    emit profileUsernameChanged();
    emit profilePasswordChanged();
    emit profileAuthgroupChanged();
    emit profileServercertChanged();
    emit profileAllowSelfSignedChanged();
    emit profileExtraArgsChanged();
}

// Getters
QString Backend::profileName() const {
    return currentProfileObject().value("name").toString();
}

QString Backend::profileServer() const {
    return currentProfileObject().value("server").toString();
}

QString Backend::profileProtocol() const {
    QString p = currentProfileObject().value("protocol").toString();
    return p.isEmpty() ? QString("anyconnect") : p;
}

QString Backend::profileUsername() const {
    return currentProfileObject().value("username").toString();
}

QString Backend::profilePassword() const {
    return currentProfileObject().value("password").toString();
}

QString Backend::profileAuthgroup() const {
    return currentProfileObject().value("authgroup").toString();
}

QString Backend::profileServercert() const {
    return currentProfileObject().value("servercert").toString();
}

bool Backend::profileAllowSelfSigned() const {
    return currentProfileObject().value("allowSelfSigned").toBool(false);
}

QString Backend::profileExtraArgs() const {
    return currentProfileObject().value("extraArgs").toString();
}

// Setters
void Backend::setProfileName(const QString &val) {
    if (profileName() != val) {
        updateCurrentProfileField("name", val);
        emit profileNameChanged();
        emit profilesChanged();
    }
}

void Backend::setProfileServer(const QString &val) {
    if (profileServer() != val) {
        updateCurrentProfileField("server", val);
        emit profileServerChanged();
    }
}

void Backend::setProfileProtocol(const QString &val) {
    if (profileProtocol() != val) {
        updateCurrentProfileField("protocol", val);
        emit profileProtocolChanged();
    }
}

void Backend::setProfileUsername(const QString &val) {
    if (profileUsername() != val) {
        updateCurrentProfileField("username", val);
        emit profileUsernameChanged();
    }
}

void Backend::setProfilePassword(const QString &val) {
    if (profilePassword() != val) {
        updateCurrentProfileField("password", val);
        emit profilePasswordChanged();
    }
}

void Backend::setProfileAuthgroup(const QString &val) {
    if (profileAuthgroup() != val) {
        updateCurrentProfileField("authgroup", val);
        emit profileAuthgroupChanged();
    }
}

void Backend::setProfileServercert(const QString &val) {
    if (profileServercert() != val) {
        updateCurrentProfileField("servercert", val);
        emit profileServercertChanged();
    }
}

void Backend::setProfileAllowSelfSigned(bool val) {
    if (profileAllowSelfSigned() != val) {
        updateCurrentProfileField("allowSelfSigned", val);
        emit profileAllowSelfSignedChanged();
    }
}

void Backend::setProfileExtraArgs(const QString &val) {
    if (profileExtraArgs() != val) {
        updateCurrentProfileField("extraArgs", val);
        emit profileExtraArgsChanged();
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
    emitActiveProfileSignals();
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
        emitActiveProfileSignals();
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

QString Backend::fetchServerCert(const QString &server) {
    QString raw = server.trimmed();
    if (raw.isEmpty()) return QString();

    QString host = raw;
    int port = 443;

    if (host.startsWith("https://", Qt::CaseInsensitive)) {
        host = host.mid(8);
    } else if (host.startsWith("http://", Qt::CaseInsensitive)) {
        host = host.mid(7);
    }

    if (host.contains("/")) {
        host = host.section('/', 0, 0);
    }

    if (host.contains(":")) {
        port = host.section(':', 1, 1).toInt();
        host = host.section(':', 0, 0);
        if (port <= 0) port = 443;
    }

    QProcess proc;
    QString cmd = QString(
        "echo | openssl s_client -connect %1:%2 2>/dev/null | "
        "openssl x509 -pubkey -noout | "
        "openssl pkey -pubin -outform der | "
        "openssl dgst -sha256 -binary | "
        "openssl enc -base64"
    ).arg(host).arg(port);

    proc.start("sh", {"-c", cmd});
    if (proc.waitForFinished(4000)) {
        QString hash = QString::fromUtf8(proc.readAllStandardOutput()).trimmed();
        if (!hash.isEmpty()) {
            return QString("pin-sha256:%1").arg(hash);
        }
    }
    return QString();
}

void Backend::fetchCertificateFingerprint() {
    QString server = profileServer();
    if (server.isEmpty()) {
        appendLog("[OpenConn] Server address is empty; cannot fetch certificate.");
        return;
    }

    appendLog(QString("[OpenConn] Querying certificate from %1...").arg(server));
    QString fp = fetchServerCert(server);
    if (!fp.isEmpty()) {
        setProfileServercert(fp);
        appendLog(QString("[OpenConn] Successfully fetched server fingerprint: %1").arg(fp));
    } else {
        appendLog("[OpenConn] Failed to retrieve server certificate. Please check server URL and port.");
    }
}

void Backend::sendInput(const QString &text) {
    appendLog(QString("[Terminal Input] > %1").arg(text));
    QByteArray data = text.toUtf8() + "\n";
    if (m_authProcess && m_authProcess->state() == QProcess::Running) {
        m_authProcess->write(data);
    }
    if (m_tunnelProcess && m_tunnelProcess->state() == QProcess::Running) {
        m_tunnelProcess->write(data);
    }
}

void Backend::acceptServerCert() {
    m_waitingForCertPrompt = false;
    emit waitingForCertPromptChanged();

    if (!m_pendingServerCert.isEmpty()) {
        setProfileServercert(m_pendingServerCert);
        appendLog(QString("[OpenConn] Trusted and saved server certificate pin: %1").arg(m_pendingServerCert));
        m_pendingServerCert.clear();
    }
    appendLog("[OpenConn] Accepting server certificate...");
    sendInput("yes");
}

void Backend::rejectServerCert() {
    m_waitingForCertPrompt = false;
    emit waitingForCertPromptChanged();
    m_pendingServerCert.clear();
    appendLog("[OpenConn] Rejected untrusted server certificate (pin discarded).");
    sendInput("no");
}

void Backend::refreshInterfaceName() {
    for (const auto &iface : QNetworkInterface::allInterfaces()) {
        const QString name = iface.name();
        if (name.startsWith("tun") || name.startsWith("tap") || name.startsWith("vpn") || name.startsWith("csco")) {
            if (m_assignedIp != "—" && !m_assignedIp.isEmpty()) {
                for (const auto &entry : iface.addressEntries()) {
                    if (entry.ip().toString() == m_assignedIp) {
                        if (m_vpnInterface != name) {
                            m_vpnInterface = name;
                            emit vpnInterfaceChanged();
                        }
                        return;
                    }
                }
            }
            if (m_vpnInterface == "—" || m_vpnInterface.isEmpty()) {
                m_vpnInterface = name;
                emit vpnInterfaceChanged();
            }
        }
    }
}

void Backend::parseLogOutput(const QString &text) {
    // Detect assigned IP:
    // Matches "Configured as 10.31.10.62" or "Connected as 10.31.10.62"
    static const QRegularExpression ipRegex(R"re((?:Connected as|Configured as)\s+([0-9]+\.[0-9]+\.[0-9]+\.[0-9]+))re");
    auto matchIp = ipRegex.match(text);
    if (matchIp.hasMatch()) {
        m_assignedIp = matchIp.captured(1);
        emit assignedIpChanged();
        refreshInterfaceName();
    }

    // Detect interface if explicitly printed in logs
    static const QRegularExpression ifRegex(R"re((?:Configured|Opened|Using device|Using interface)\s+(?:device\s+)?(tun[0-9]+|tap[0-9]+|vpn[0-9]+|cscotun[0-9]+))re");
    auto matchIf = ifRegex.match(text);
    if (matchIf.hasMatch()) {
        m_vpnInterface = matchIf.captured(1);
        emit vpnInterfaceChanged();
    }

    // Detect servercert suggestion from openconnect
    static const QRegularExpression certRegex(R"re(--servercert\s+(pin-sha256:[^\s\r\n]+|[0-9a-fA-F]{40,}))re");
    auto matchCert = certRegex.match(text);
    if (matchCert.hasMatch()) {
        m_pendingServerCert = matchCert.captured(1);
        appendLog(QString("[OpenConn] Candidate server certificate detected: %1 (pending user acceptance)").arg(m_pendingServerCert));
    }

    // Detect certificate confirmation prompt
    if (text.contains("Enter 'yes' to accept") || text.contains("failed verification")) {
        m_waitingForCertPrompt = true;
        emit waitingForCertPromptChanged();

        if (profileAllowSelfSigned()) {
            appendLog("[OpenConn] Auto-accepting server certificate (Allow self-signed is ON)...");
            acceptServerCert();
        }
    }

    // Detect successful tunnel establishment:
    // Matches OpenConnect indicators: "Configured as ", "CSTP connected", "Session authentication will expire", etc.
    if (text.contains("Configured as ") ||
        text.contains("CSTP connected") ||
        text.contains("Established DTLS connection") ||
        text.contains("Connected as ") ||
        text.contains("Session established") ||
        text.contains("SSL connected") ||
        text.contains("Session authentication will expire") ||
        text.contains("Using vhost-net for tun acceleration")) {
        m_waitingForCertPrompt = false;
        emit waitingForCertPromptChanged();

        if (m_state != Connected) {
            setState(Connected, "Connected");
            m_connectedTimestamp = QDateTime::currentSecsSinceEpoch();
            m_durationTimer->start();
            refreshInterfaceName();
            QTimer::singleShot(600, this, [this]() {
                refreshInterfaceName();
            });
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

    QString server = profileServer().trimmed();
    if (server.isEmpty()) {
        setState(Error, "Server URL is required");
        appendLog("[Error] Please configure a Server/Gateway address before connecting.");
        return;
    }

    // Auto-fetch cert fingerprint if self-signed is allowed and field is blank
    if (profileServercert().isEmpty() && profileAllowSelfSigned()) {
        appendLog("[OpenConn] Querying certificate fingerprint for untrusted server...");
        QString cert = fetchServerCert(server);
        if (!cert.isEmpty()) {
            setProfileServercert(cert);
            appendLog(QString("[OpenConn] Trusted server fingerprint: %1").arg(cert));
        }
    }

    startPhase1Auth();
}

void Backend::startPhase1Auth() {
    QString server = profileServer().trimmed();
    QString protocol = profileProtocol().trimmed();
    QString username = profileUsername().trimmed();
    QString password = profilePassword();
    QString authgroup = profileAuthgroup().trimmed();
    QString servercert = profileServercert().trimmed();

    m_waitingForCertPrompt = false;
    m_pendingServerCert.clear();
    emit waitingForCertPromptChanged();

    setState(Connecting, "Authenticating with gateway...");
    m_assignedIp = "—";
    m_vpnInterface = "—";
    m_connectedTime = "00:00:00";
    emit assignedIpChanged();
    emit vpnInterfaceChanged();
    emit connectedTimeChanged();

    appendLog(QString("[OpenConn] Authenticating with %1 (%2)...").arg(server, protocol));

    if (m_authProcess) {
        m_authProcess->deleteLater();
        m_authProcess = nullptr;
    }
    if (m_tunnelProcess) {
        m_tunnelProcess->deleteLater();
        m_tunnelProcess = nullptr;
    }

    m_authStdoutAccumulator.clear();
    m_authProcess = new QProcess(this);

    QStringList arguments;
    arguments << server;
    if (!protocol.isEmpty()) {
        arguments << QString("--protocol=%1").arg(protocol);
    }
    if (!username.isEmpty()) {
        arguments << QString("--user=%1").arg(username);
    }
    arguments << "--passwd-on-stdin";
    arguments << "--authenticate";
    if (!authgroup.isEmpty()) {
        arguments << QString("--authgroup=%1").arg(authgroup);
    }
    if (!servercert.isEmpty()) {
        arguments << QString("--servercert=%1").arg(servercert);
    }

    connect(m_authProcess, &QProcess::readyReadStandardOutput, this, &Backend::onAuthReadyRead);
    connect(m_authProcess, &QProcess::readyReadStandardError, this, &Backend::onAuthReadyRead);
    connect(m_authProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Backend::onAuthFinished);
    connect(m_authProcess, &QProcess::errorOccurred, this, &Backend::onProcessError);

    // Launch Phase 1 directly in user space
    m_authProcess->start("/usr/bin/openconnect", arguments);

    if (m_authProcess->waitForStarted(2000)) {
        if (!password.isEmpty()) {
            m_authProcess->write(password.toUtf8() + "\n");
        }
    }
}

void Backend::onAuthReadyRead() {
    if (!m_authProcess) return;

    QByteArray out = m_authProcess->readAllStandardOutput();
    if (!out.isEmpty()) {
        QString text = QString::fromUtf8(out);
        m_authStdoutAccumulator += text;
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            appendLog(line);
        }
    }

    QByteArray err = m_authProcess->readAllStandardError();
    if (!err.isEmpty()) {
        QString text = QString::fromUtf8(err);
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            appendLog(line);
        }
    }
}

void Backend::onAuthFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    if (m_state != Connecting) return;

    if (exitCode != 0 || !m_authStdoutAccumulator.contains("COOKIE=")) {
        appendLog(QString("[OpenConn] Phase 1 authentication failed (exit code %1).").arg(exitCode));
        setState(Error, "Authentication failed. Check credentials/server.");
        return;
    }

    appendLog("[OpenConn] Authentication successful! Session cookie obtained.");

    // Parse variables from Phase 1 output
    static const QRegularExpression cookieRegex(R"re(COOKIE='([^']+)')re");
    static const QRegularExpression urlRegex(R"re(CONNECT_URL='([^']+)')re");
    static const QRegularExpression fpRegex(R"re(FINGERPRINT='([^']+)')re");
    static const QRegularExpression resolveRegex(R"re(RESOLVE='([^']+)')re");

    auto mCookie = cookieRegex.match(m_authStdoutAccumulator);
    auto mUrl = urlRegex.match(m_authStdoutAccumulator);
    auto mFp = fpRegex.match(m_authStdoutAccumulator);
    auto mResolve = resolveRegex.match(m_authStdoutAccumulator);

    QString cookie = mCookie.hasMatch() ? mCookie.captured(1) : QString();
    QString connectUrl = mUrl.hasMatch() ? mUrl.captured(1) : profileServer();
    QString fingerprint = mFp.hasMatch() ? mFp.captured(1) : profileServercert();
    QString resolve = mResolve.hasMatch() ? mResolve.captured(1) : QString();

    if (cookie.isEmpty()) {
        setState(Error, "Could not extract session cookie.");
        return;
    }

    startPhase2Tunnel(cookie, connectUrl, fingerprint, resolve);
}

void Backend::startPhase2Tunnel(const QString &cookie, const QString &connectUrl,
                                const QString &fingerprint, const QString &resolve) {
    setState(Connecting, "Establishing VPN tunnel (elevating)...");
    appendLog("[OpenConn] Launching elevated tunnel connection with session cookie...");

    m_tunnelProcess = new QProcess(this);

    QString currentUser = QString::fromUtf8(qgetenv("USER"));
    if (currentUser.isEmpty()) currentUser = "anthony";

    QStringList arguments;
    arguments << "/usr/bin/openconnect";
    arguments << "--cookie-on-stdin";
    arguments << connectUrl;
    if (!fingerprint.isEmpty()) {
        arguments << QString("--servercert=%1").arg(fingerprint);
    }
    if (!resolve.isEmpty()) {
        arguments << QString("--resolve=%1").arg(resolve);
    }
    arguments << QString("--setuid=%1").arg(currentUser);
    arguments << QString("--pid-file=/tmp/openconn.pid");

    QString extraArgs = profileExtraArgs().trimmed();
    if (!extraArgs.isEmpty()) {
        arguments << extraArgs.split(" ", Qt::SkipEmptyParts);
    }

    connect(m_tunnelProcess, &QProcess::readyReadStandardOutput, this, &Backend::onTunnelReadyRead);
    connect(m_tunnelProcess, &QProcess::readyReadStandardError, this, &Backend::onTunnelReadyRead);
    connect(m_tunnelProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &Backend::onTunnelFinished);
    connect(m_tunnelProcess, &QProcess::errorOccurred, this, &Backend::onProcessError);

    // Launch Phase 2 elevated with pkexec passing cookie securely via stdin
    m_tunnelProcess->start("pkexec", arguments);
    if (m_tunnelProcess->waitForStarted(3000)) {
        m_tunnelProcess->write(cookie.toUtf8() + "\n");
    }
}

void Backend::onTunnelReadyRead() {
    if (!m_tunnelProcess) return;

    QByteArray out = m_tunnelProcess->readAllStandardOutput();
    if (!out.isEmpty()) {
        QString text = QString::fromUtf8(out);
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            appendLog(line);
        }
    }

    QByteArray err = m_tunnelProcess->readAllStandardError();
    if (!err.isEmpty()) {
        QString text = QString::fromUtf8(err);
        const auto lines = text.split('\n', Qt::SkipEmptyParts);
        for (const auto &line : lines) {
            appendLog(line);
        }
    }
}

void Backend::onTunnelFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    Q_UNUSED(exitStatus);
    m_durationTimer->stop();
    appendLog(QString("[OpenConn] Tunnel process terminated (exit code: %1).").arg(exitCode));

    if (m_state == Disconnecting || exitCode == 0) {
        setState(Disconnected, "Disconnected");
    } else {
        setState(Error, QString("Tunnel closed (code %1)").arg(exitCode));
    }
    m_assignedIp = "—";
    m_vpnInterface = "—";
    emit assignedIpChanged();
    emit vpnInterfaceChanged();
}

void Backend::disconnectVpn() {
    if (m_state == Disconnected) return;

    m_waitingForCertPrompt = false;
    emit waitingForCertPromptChanged();

    setState(Disconnecting, "Disconnecting...");
    m_durationTimer->stop();
    appendLog("[OpenConn] Terminating connection...");

    // Send SIGINT directly to the openconnect PID from pid-file
    QFile pidFile("/tmp/openconn.pid");
    if (pidFile.open(QIODevice::ReadOnly)) {
        pid_t ocPid = pidFile.readAll().trimmed().toInt();
        pidFile.close();
        if (ocPid > 0) {
            ::kill(ocPid, SIGINT);
        }
        QFile::remove("/tmp/openconn.pid");
    }

    // Also ensure any lingering user-space openconnect process receives SIGINT
    QString currentUser = QString::fromUtf8(qgetenv("USER"));
    if (currentUser.isEmpty()) currentUser = "anthony";
    QProcess::execute("pkill", {"-2", "-u", currentUser, "-x", "openconnect"});

    if (m_authProcess && m_authProcess->state() != QProcess::NotRunning) {
        m_authProcess->terminate();
        m_authProcess->kill();
    }

    if (m_tunnelProcess && m_tunnelProcess->state() != QProcess::NotRunning) {
        pid_t pid = m_tunnelProcess->processId();
        if (pid > 0) {
            ::kill(pid, SIGINT);
        }
        m_tunnelProcess->terminate();

        QTimer::singleShot(2500, this, [this]() {
            if (m_tunnelProcess && m_tunnelProcess->state() != QProcess::NotRunning) {
                m_tunnelProcess->kill();
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

void Backend::onProcessError(QProcess::ProcessError error) {
    if (m_state == Disconnecting) return;

    m_waitingForCertPrompt = false;
    emit waitingForCertPromptChanged();

    QString errorMsg;
    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "Failed to launch OpenConnect process. Check if openconnect/pkexec is installed.";
        break;
    case QProcess::Crashed:
        errorMsg = "OpenConnect process exited unexpectedly.";
        break;
    case QProcess::Timedout:
        errorMsg = "Process operation timed out.";
        break;
    default:
        errorMsg = "Process error occurred.";
        break;
    }

    appendLog(QString("[OpenConn Error] %1").arg(errorMsg));
    setState(Error, errorMsg);
}

void Backend::onTimerTick() {
    if (m_state != Connected || m_connectedTimestamp <= 0) return;

    if (m_vpnInterface == "—" || m_vpnInterface.isEmpty()) {
        refreshInterfaceName();
    }

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
