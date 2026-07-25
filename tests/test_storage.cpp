#include "credentialstore.h"
#include "settingsstore.h"

#include <QFile>
#include <QTemporaryDir>
#include <QTest>

using namespace KHeadless;

class StorageTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void credentialsUseOneWayHashes()
    {
        QTemporaryDir directory;
        const auto path = directory.filePath(QStringLiteral("credentials.json"));
        CredentialStore store(path);
        QString error;
        QVERIFY(store.setPassword(QStringLiteral("alice"), QStringLiteral("correct horse"), &error));
        QVERIFY(store.save(&error));
        QVERIFY(store.verify(QStringLiteral("alice"), QStringLiteral("correct horse")));
        QVERIFY(!store.verify(QStringLiteral("alice"), QStringLiteral("wrong password")));

        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const auto contents = file.readAll();
        QVERIFY(!contents.contains("correct horse"));
        QVERIFY(contents.contains("$argon2id$"));

        CredentialStore reloaded(path);
        QVERIFY(reloaded.load(&error));
        QVERIFY(reloaded.verify(QStringLiteral("alice"), QStringLiteral("correct horse")));
    }

    void settingsAndProfilesRoundTrip()
    {
        QTemporaryDir directory;
        const auto path = directory.filePath(QStringLiteral("config.json"));
        SettingsStore store(path);
        store.setMode(QStringLiteral("manual"));
        auto layout = MonitorLayout::fallback();
        layout.monitors[0].width = 2560;
        store.setLayout(layout);
        QVERIFY(store.saveProfile(QStringLiteral("wide"), layout));
        QVERIFY(store.save());

        SettingsStore reloaded(path);
        QString error;
        QVERIFY(reloaded.load(&error));
        QCOMPARE(reloaded.mode(), QStringLiteral("manual"));
        QCOMPARE(reloaded.layout(), layout);
        QCOMPARE(reloaded.profiles().value(QStringLiteral("wide")), layout);
    }
};

QTEST_MAIN(StorageTest)
#include "test_storage.moc"

