#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"

class OpenConnTests : public QObject {
    Q_OBJECT

private slots:
    void testProfileCreationAndDefaults() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Backend backend(dir.path());
        QCOMPARE(backend.profiles().size(), 1);
        QCOMPARE(backend.profileName(), QString("My VPN"));
        QCOMPARE(backend.profileProtocol(), QString("anyconnect"));
    }

    void testReactivePropertySettersAndPersistence() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        {
            Backend backend(dir.path());
            backend.setProfileName("Production Gateway");
            backend.setProfileServer("vpn.prod.net");
            backend.setProfileUsername("alice");
            backend.setProfilePassword("secret123");
            backend.setProfileProtocol("gp");

            QCOMPARE(backend.profileName(), QString("Production Gateway"));
            QCOMPARE(backend.profileServer(), QString("vpn.prod.net"));
            QCOMPARE(backend.profileUsername(), QString("alice"));
            QCOMPARE(backend.profilePassword(), QString("secret123"));
            QCOMPARE(backend.profileProtocol(), QString("gp"));
        }

        {
            Backend reloaded(dir.path());
            QCOMPARE(reloaded.profiles().size(), 1);
            QCOMPARE(reloaded.profileName(), QString("Production Gateway"));
            QCOMPARE(reloaded.profileServer(), QString("vpn.prod.net"));
            QCOMPARE(reloaded.profileUsername(), QString("alice"));
            QCOMPARE(reloaded.profilePassword(), QString("secret123"));
            QCOMPARE(reloaded.profileProtocol(), QString("gp"));
        }
    }

    void testAddAndRemoveProfiles() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Backend backend(dir.path());
        backend.addProfile("Secondary VPN");
        QCOMPARE(backend.profiles().size(), 2);
        QCOMPARE(backend.activeIndex(), 1);
        QCOMPARE(backend.profileName(), QString("Secondary VPN"));

        backend.removeProfile(1);
        QCOMPARE(backend.profiles().size(), 1);
        QCOMPARE(backend.activeIndex(), 0);
    }

    void testThemeAccentFallback() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#2563EB"));
        QCOMPARE(theme.accentForeground(), QString("#FFFFFF"));
    }

    void testThemeAccentParsing() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        QDir(dir.path()).mkpath("theme");
        QFile colorsFile(dir.path() + "/theme/colors.toml");
        QVERIFY(colorsFile.open(QIODevice::WriteOnly | QIODevice::Text));
        colorsFile.write("accent = \"#FFD60A\"\n");
        colorsFile.close();

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("#0F172A"));
    }

    void testConfiguredAsConnectionDetection() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Backend backend(dir.path());
        QCOMPARE(backend.connectionState(), Backend::Disconnected);

        // Simulate OpenConnect output line from user's session
        backend.simulateLogLine("Configured as 10.31.10.62, with SSL connected and DTLS in progress");

        QCOMPARE(backend.connectionState(), Backend::Connected);
        QCOMPARE(backend.statusText(), QString("Connected"));
        QCOMPARE(backend.assignedIp(), QString("10.31.10.62"));
    }

    void testCertificatePinningOnlyOnAcceptAndDiscardOnReject() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Backend backend(dir.path());
        QCOMPARE(backend.profileServercert(), QString(""));

        // 1. Candidate cert detected from OpenConnect log output
        backend.simulateLogLine("To trust this server in future, perhaps add this to your command line: --servercert pin-sha256:FAKEHASH123=");
        // The cert MUST NOT be auto-saved into the profile before user approval
        QCOMPARE(backend.profileServercert(), QString(""));

        // 2. User rejects the certificate
        backend.rejectServerCert();
        // The profile remains clean and untrusted
        QCOMPARE(backend.profileServercert(), QString(""));

        // 3. Next attempt prompts with legitimate cert pin
        backend.simulateLogLine("To trust this server in future, perhaps add this to your command line: --servercert pin-sha256:TRUSTEDHASH456=");
        QCOMPARE(backend.profileServercert(), QString(""));

        // 4. User explicitly accepts
        backend.acceptServerCert();
        // Now the trusted pin is saved to the profile
        QCOMPARE(backend.profileServercert(), QString("pin-sha256:TRUSTEDHASH456="));
    }
};

QTEST_MAIN(OpenConnTests)
#include "openconn_tests.moc"
