#include "rdplayoutconversion.h"

#include <QTest>

using namespace KHeadless;

class RdpLayoutTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void preservesTopologyAndAttributes()
    {
        MonitorLayout source{{
            {QStringLiteral("left"), QStringLiteral("Left"), -1920, 0, 1920, 1080, 1.0, 0, true, false},
            {QStringLiteral("primary"), QStringLiteral("Primary"), 0, 0, 2560, 1440, 1.25, 90, true, true},
            {QStringLiteral("upper"), QStringLiteral("Upper"), 0, -1080, 1920, 1080, 1.5, 270, true, false},
        }};
        QVERIFY2(source.validate().valid, qPrintable(source.validate().error));

        const auto wire = toKrdpLayout(source);
        QCOMPARE(wire.size(), 3);
        QCOMPARE(wire.at(0).position, QPoint(-1920, 0));
        QCOMPARE(wire.at(1).size, QSize(2560, 1440));
        QCOMPARE(wire.at(1).desktopScaleFactor, 125U);
        QCOMPARE(wire.at(1).orientation, 90U);
        QVERIFY(wire.at(1).primary);

        const auto decoded = fromKrdpLayout(wire);
        QCOMPARE(decoded.monitors.size(), 3);
        QCOMPARE(decoded.monitors.at(0).x, -1920);
        QCOMPARE(decoded.monitors.at(1).scale, 1.25);
        QCOMPARE(decoded.monitors.at(2).rotation, 270);
        QVERIFY(decoded.monitors.at(1).primary);
        QVERIFY2(decoded.validate().valid, qPrintable(decoded.validate().error));
    }

    void omitsDisabledOutputs()
    {
        auto source = MonitorLayout::fallback();
        source.monitors.append({
            .id = QStringLiteral("disabled"),
            .name = QStringLiteral("Disabled"),
            .x = 1920,
            .y = 0,
            .width = 1280,
            .height = 720,
            .scale = 1.0,
            .rotation = 0,
            .enabled = false,
            .primary = false,
        });

        const auto wire = toKrdpLayout(source);
        QCOMPARE(wire.size(), 1);
        QVERIFY(wire.constFirst().primary);
    }
};

QTEST_GUILESS_MAIN(RdpLayoutTest)

#include "test_rdplayout.moc"
