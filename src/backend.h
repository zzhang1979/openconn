#pragma once

#include <QDateTime>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QProcess>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>

class Backend : public QObject {
    Q_OBJECT

    Q_PROPERTY(int connectionState READ connectionState NOTIFY connectionStateChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString connectedTime READ connectedTime NOTIFY connectedTimeChanged)
    Q_PROPERTY(QString assignedIp READ assignedIp NOTIFY assignedIpChanged)
    Q_PROPERTY(QString vpnInterface READ vpnInterface NOTIFY vpnInterfaceChanged)
    Q_PROPERTY(QVariantList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(int activeIndex READ activeIndex WRITE setActiveIndex NOTIFY activeIndexChanged)
    Q_PROPERTY(QString logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(bool hasPolkitRule READ hasPolkitRule NOTIFY hasPolkitRuleChanged)

    // Dedicated direct reactive properties for the active profile fields
    Q_PROPERTY(QString profileName READ profileName WRITE setProfileName NOTIFY profileNameChanged)
    Q_PROPERTY(QString profileServer READ profileServer WRITE setProfileServer NOTIFY profileServerChanged)
    Q_PROPERTY(QString profileProtocol READ profileProtocol WRITE setProfileProtocol NOTIFY profileProtocolChanged)
    Q_PROPERTY(QString profileUsername READ profileUsername WRITE setProfileUsername NOTIFY profileUsernameChanged)
    Q_PROPERTY(QString profilePassword READ profilePassword WRITE setProfilePassword NOTIFY profilePasswordChanged)
    Q_PROPERTY(QString profileAuthgroup READ profileAuthgroup WRITE setProfileAuthgroup NOTIFY profileAuthgroupChanged)
    Q_PROPERTY(QString profileServercert READ profileServercert WRITE setProfileServercert NOTIFY profileServercertChanged)
    Q_PROPERTY(bool profileAllowSelfSigned READ profileAllowSelfSigned WRITE setProfileAllowSelfSigned NOTIFY profileAllowSelfSignedChanged)
    Q_PROPERTY(QString profileExtraArgs READ profileExtraArgs WRITE setProfileExtraArgs NOTIFY profileExtraArgsChanged)

public:
    enum State {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        Disconnecting = 3,
        Error = 4
    };
    Q_ENUM(State)

    explicit Backend(const QString &configDir = QString(), QObject *parent = nullptr);
    ~Backend() override;

    int connectionState() const { return m_state; }
    QString statusText() const { return m_statusText; }
    QString connectedTime() const { return m_connectedTime; }
    QString assignedIp() const { return m_assignedIp; }
    QString vpnInterface() const { return m_vpnInterface; }
    QVariantList profiles() const;
    int activeIndex() const { return m_activeIndex; }
    QString logs() const { return m_logs; }
    bool hasPolkitRule() const { return m_hasPolkitRule; }

    // Active profile field getters
    QString profileName() const;
    QString profileServer() const;
    QString profileProtocol() const;
    QString profileUsername() const;
    QString profilePassword() const;
    QString profileAuthgroup() const;
    QString profileServercert() const;
    bool profileAllowSelfSigned() const;
    QString profileExtraArgs() const;

    // Active profile field setters
    void setProfileName(const QString &val);
    void setProfileServer(const QString &val);
    void setProfileProtocol(const QString &val);
    void setProfileUsername(const QString &val);
    void setProfilePassword(const QString &val);
    void setProfileAuthgroup(const QString &val);
    void setProfileServercert(const QString &val);
    void setProfileAllowSelfSigned(bool val);
    void setProfileExtraArgs(const QString &val);

    void setActiveIndex(int index);

    Q_INVOKABLE void connectVpn();
    Q_INVOKABLE void disconnectVpn();
    Q_INVOKABLE void toggleConnect();
    Q_INVOKABLE void addProfile(const QString &name = QString());
    Q_INVOKABLE void removeProfile(int index);
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE QString getPolkitRuleSnippet() const;

signals:
    void connectionStateChanged();
    void statusTextChanged();
    void connectedTimeChanged();
    void assignedIpChanged();
    void vpnInterfaceChanged();
    void profilesChanged();
    void activeIndexChanged();
    void logsChanged();
    void hasPolkitRuleChanged();

    void profileNameChanged();
    void profileServerChanged();
    void profileProtocolChanged();
    void profileUsernameChanged();
    void profilePasswordChanged();
    void profileAuthgroupChanged();
    void profileServercertChanged();
    void profileAllowSelfSignedChanged();
    void profileExtraArgsChanged();

private slots:
    void onProcessReadyRead();
    void onProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onProcessError(QProcess::ProcessError error);
    void onTimerTick();

private:
    void setState(State newState, const QString &text = QString());
    void appendLog(const QString &line);
    void loadProfiles();
    void saveProfiles();
    void checkPolkitRule();
    void parseLogOutput(const QString &text);
    void emitActiveProfileSignals();

    QJsonObject currentProfileObject() const;
    void updateCurrentProfileField(const QString &key, const QJsonValue &value);

    QString m_configDir;
    QString m_configFile;
    QJsonArray m_profilesData;
    int m_activeIndex{0};

    State m_state{Disconnected};
    QString m_statusText{"Ready"};
    QString m_connectedTime{"00:00:00"};
    QString m_assignedIp{"—"};
    QString m_vpnInterface{"—"};
    QString m_logs;
    bool m_hasPolkitRule{false};

    QProcess *m_process{nullptr};
    QTimer *m_durationTimer{nullptr};
    qint64 m_connectedTimestamp{0};
};
