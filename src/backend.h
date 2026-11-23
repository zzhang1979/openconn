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
    Q_PROPERTY(QVariantMap activeProfile READ activeProfile NOTIFY activeProfileChanged)
    Q_PROPERTY(QString logs READ logs NOTIFY logsChanged)
    Q_PROPERTY(bool hasPolkitRule READ hasPolkitRule NOTIFY hasPolkitRuleChanged)

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
    QVariantMap activeProfile() const;
    QString logs() const { return m_logs; }
    bool hasPolkitRule() const { return m_hasPolkitRule; }

    void setActiveIndex(int index);

    Q_INVOKABLE void connectVpn();
    Q_INVOKABLE void disconnectVpn();
    Q_INVOKABLE void toggleConnect();
    Q_INVOKABLE void addProfile(const QString &name = QString());
    Q_INVOKABLE void removeProfile(int index);
    Q_INVOKABLE void updateProfile(int index, const QVariantMap &data);
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
    void activeProfileChanged();
    void logsChanged();
    void hasPolkitRuleChanged();

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
