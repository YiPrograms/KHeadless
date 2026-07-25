#include "layoutcontroller.h"
#include "outputbackend.h"

#include <QTest>

using namespace KHeadless;

class LayoutControllerTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void appliesAndConfirms()
    {
        auto backend = std::make_unique<MemoryOutputBackend>();
        LayoutController controller(std::move(backend));
        auto layout = MonitorLayout::fallback();
        layout.monitors[0].width = 2560;
        QString error;
        QVERIFY(controller.apply(layout, true, &error));
        QVERIFY(controller.hasPendingConfirmation());
        QCOMPARE(controller.layout(), layout);
        QVERIFY(controller.confirm());
        QVERIFY(!controller.hasPendingConfirmation());
    }

    void revertsPendingLayout()
    {
        auto backend = std::make_unique<MemoryOutputBackend>();
        LayoutController controller(std::move(backend));
        const auto original = controller.layout();
        auto changed = original;
        changed.monitors[0].height = 1440;
        QVERIFY(controller.apply(changed, true));
        QVERIFY(controller.revert());
        QCOMPARE(controller.layout(), original);
    }

    void rejectsInvalidLayoutWithoutMutation()
    {
        auto backend = std::make_unique<MemoryOutputBackend>();
        LayoutController controller(std::move(backend));
        const auto original = controller.layout();
        auto invalid = original;
        invalid.monitors[0].width = 10;
        QString error;
        QVERIFY(!controller.apply(invalid, false, &error));
        QVERIFY(!error.isEmpty());
        QCOMPARE(controller.layout(), original);
    }

    void adoptsConfiguredLayoutBeforeOutputsExist()
    {
        auto backend = std::make_unique<MemoryOutputBackend>();
        LayoutController controller(std::move(backend));
        auto layout = MonitorLayout::fallback();
        layout.monitors[0].width = 2560;
        QVERIFY(controller.setInitialLayout(layout));
        QCOMPARE(controller.layout(), layout);
        QCOMPARE(controller.previousLayout(), layout);
        QVERIFY(!controller.hasPendingConfirmation());
    }
};

QTEST_MAIN(LayoutControllerTest)
#include "test_layoutcontroller.moc"
