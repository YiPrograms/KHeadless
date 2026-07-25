#include "rdplayoutconversion.h"

#include <QtMath>

using namespace KHeadless;

KRdp::DisplayMonitorList KHeadless::toKrdpLayout(const MonitorLayout &layout)
{
    KRdp::DisplayMonitorList result;
    for (const auto &monitor : layout.monitors) {
        if (!monitor.enabled) {
            continue;
        }
        result.append({
            .position = QPoint(monitor.x, monitor.y),
            .size = QSize(monitor.width, monitor.height),
            .orientation = uint32_t(monitor.rotation),
            .desktopScaleFactor = uint32_t(qRound(monitor.scale * 100.0)),
            .deviceScaleFactor = 100,
            .primary = monitor.primary,
        });
    }
    return result;
}

MonitorLayout KHeadless::fromKrdpLayout(const KRdp::DisplayMonitorList &layout)
{
    MonitorLayout result;
    result.monitors.reserve(layout.size());
    for (qsizetype index = 0; index < layout.size(); ++index) {
        const auto &monitor = layout.at(index);
        result.monitors.append({
            .id = QStringLiteral("KHEADLESS-%1").arg(index + 1),
            .name = QStringLiteral("Display %1").arg(index + 1),
            .x = monitor.position.x(),
            .y = monitor.position.y(),
            .width = monitor.size.width(),
            .height = monitor.size.height(),
            .scale = monitor.desktopScaleFactor / 100.0,
            .rotation = int(monitor.orientation),
            .enabled = true,
            .primary = monitor.primary,
        });
    }
    return result;
}
