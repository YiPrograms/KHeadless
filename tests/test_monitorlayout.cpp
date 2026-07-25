#include "monitorlayout.h"

#include <QTest>

using namespace KHeadless;

class MonitorLayoutTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void fallbackIsValid()
    {
        QVERIFY(MonitorLayout::fallback().validate().valid);
    }

    void acceptsNegativeCoordinatesAndNormalizes()
    {
        MonitorLayout layout{{
            {QStringLiteral("left"), QStringLiteral("Left"), -1920, 100, 1920, 1080, 1.0, 0, true, false},
            {QStringLiteral("main"), QStringLiteral("Main"), 0, 0, 2560, 1440, 1.25, 0, true, true},
        }};
        QVERIFY2(layout.validate().valid, qPrintable(layout.validate().error));
        const auto normalized = layout.normalized();
        QCOMPARE(normalized.monitors.at(0).x, 0);
        QCOMPARE(normalized.monitors.at(0).y, 100);
        QCOMPARE(normalized.monitors.at(1).x, 1920);
        QCOMPARE(normalized.monitors.at(1).y, 0);
    }

    void rejectsOverlap()
    {
        MonitorLayout layout{{
            {QStringLiteral("one"), QStringLiteral("One"), 0, 0, 1920, 1080, 1.0, 0, true, true},
            {QStringLiteral("two"), QStringLiteral("Two"), 1000, 0, 1920, 1080, 1.0, 0, true, false},
        }};
        QVERIFY(!layout.validate().valid);
        QVERIFY(layout.validate().error.contains(QStringLiteral("overlaps")));
    }

    void rejectsInvalidPrimaryAndDimensions()
    {
        auto layout = MonitorLayout::fallback();
        layout.monitors[0].primary = false;
        QVERIFY(!layout.validate().valid);
        layout.monitors[0].primary = true;
        layout.monitors[0].width = 100;
        QVERIFY(!layout.validate().valid);
    }

    void roundTripsVariant()
    {
        const auto source = MonitorLayout::fallback();
        QString error;
        const auto copy = MonitorLayout::fromVariantList(source.toVariantList(), &error);
        QVERIFY2(error.isEmpty(), qPrintable(error));
        QCOMPARE(copy, source);
    }

    void acceptsOneThroughSixteenOutputs()
    {
        for (int count = 1; count <= MonitorLayout::MaximumMonitors; ++count) {
            MonitorLayout layout;
            for (int index = 0; index < count; ++index) {
                layout.monitors.append({
                    .id = QStringLiteral("display-%1").arg(index),
                    .name = QStringLiteral("Display %1").arg(index),
                    .x = index * 1920,
                    .y = 0,
                    .width = 1920,
                    .height = 1080,
                    .scale = 0.5 + (index % 8) * 0.5,
                    .rotation = (index % 4) * 90,
                    .enabled = true,
                    .primary = index == 0,
                });
            }
            const auto validation = layout.validate();
            QVERIFY2(validation.valid, qPrintable(validation.error));
        }
    }

    void rejectsSeventeenOutputs()
    {
        MonitorLayout layout;
        for (int index = 0; index <= MonitorLayout::MaximumMonitors; ++index) {
            layout.monitors.append({
                .id = QStringLiteral("display-%1").arg(index),
                .name = QStringLiteral("Display %1").arg(index),
                .x = index * 200,
                .y = 0,
                .width = 200,
                .height = 200,
                .scale = 1.0,
                .rotation = 0,
                .enabled = true,
                .primary = index == 0,
            });
        }
        QVERIFY(!layout.validate().valid);
    }
};

QTEST_MAIN(MonitorLayoutTest)
#include "test_monitorlayout.moc"
