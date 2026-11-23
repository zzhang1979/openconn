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

        QVariantMap initial = backend.activeProfile();
        QCOMPARE(initial.value("name").toString(), QString("My VPN"));
        QCOMPARE(initial.value("protocol").toString(), QString("anyconnect"));
    }

    void testProfilePersistence() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        {
            Backend backend(dir.path());
            QVariantMap p = backend.activeProfile();
            p["name"] = "Production Gateway";
            p["server"] = "vpn.prod.net";
            p["username"] = "alice";
            p["password"] = "secret123";
            p["protocol"] = "gp";
            backend.updateProfile(0, p);
        }

        {
            Backend reloaded(dir.path());
            QCOMPARE(reloaded.profiles().size(), 1);
            QVariantMap p = reloaded.activeProfile();
            QCOMPARE(p.value("name").toString(), QString("Production Gateway"));
            QCOMPARE(p.value("server").toString(), QString("vpn.prod.net"));
            QCOMPARE(p.value("username").toString(), QString("alice"));
            QCOMPARE(p.value("password").toString(), QString("secret123"));
            QCOMPARE(p.value("protocol").toString(), QString("gp"));
        }
    }

    void testAddAndRemoveProfiles() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());

        Backend backend(dir.path());
        backend.addProfile("Secondary VPN");
        QCOMPARE(backend.profiles().size(), 2);
        QCOMPARE(backend.activeIndex(), 1);
        QCOMPARE(backend.activeProfile().value("name").toString(), QString("Secondary VPN"));

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
        // Light yellow accent requires dark foreground
        QCOMPARE(theme.accentForeground(), QString("#0F172A"));
    }
};

QTEST_MAIN(OpenConnTests)
#include "openconn_tests.moc"
