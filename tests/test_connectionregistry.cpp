#include "connectionregistry.h"

#include <QTest>

using namespace KHeadless;

class ConnectionRegistryTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void newestWritableClientOwnsSession()
    {
        ConnectionRegistry registry;
        const auto first = registry.add(QStringLiteral("alice"), QStringLiteral("10.0.0.1"));
        QCOMPARE(registry.controllerId(), first);
        const auto viewer = registry.add(QStringLiteral("viewer"), QStringLiteral("10.0.0.2"), true);
        Q_UNUSED(viewer)
        QCOMPARE(registry.controllerId(), first);
        const auto second = registry.add(QStringLiteral("bob"), QStringLiteral("10.0.0.3"));
        QCOMPARE(registry.controllerId(), second);
        QVERIFY(registry.remove(second));
        QCOMPARE(registry.controllerId(), first);
    }

    void storesClientLayout()
    {
        ConnectionRegistry registry;
        const auto id = registry.add(QStringLiteral("alice"), QStringLiteral("local"));
        auto layout = MonitorLayout::fallback();
        layout.monitors[0].width = 2560;
        QVERIFY(registry.setRequestedLayout(id, layout));
        QCOMPARE(registry.connections().first().requestedLayout, layout);
    }

    void preservesBackendConnectionId()
    {
        ConnectionRegistry registry;
        QCOMPARE(registry.add(QStringLiteral("alice"),
                              QStringLiteral("local"),
                              false,
                              QStringLiteral("rdp-peer-42")),
                 QStringLiteral("rdp-peer-42"));
        QCOMPARE(registry.controllerId(), QStringLiteral("rdp-peer-42"));
    }
};

QTEST_MAIN(ConnectionRegistryTest)
#include "test_connectionregistry.moc"
