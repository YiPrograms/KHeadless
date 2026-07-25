#include "monitorlayout.h"

#include <QJsonArray>
#include <QRect>
#include <QSet>

#include <algorithm>

using namespace KHeadless;

namespace
{
QVariantMap monitorToMap(const Monitor &monitor)
{
    return {
        {QStringLiteral("id"), monitor.id},
        {QStringLiteral("name"), monitor.name},
        {QStringLiteral("x"), monitor.x},
        {QStringLiteral("y"), monitor.y},
        {QStringLiteral("width"), monitor.width},
        {QStringLiteral("height"), monitor.height},
        {QStringLiteral("scale"), monitor.scale},
        {QStringLiteral("rotation"), monitor.rotation},
        {QStringLiteral("enabled"), monitor.enabled},
        {QStringLiteral("primary"), monitor.primary},
    };
}

Monitor monitorFromMap(const QVariantMap &map)
{
    Monitor monitor;
    monitor.id = map.value(QStringLiteral("id")).toString();
    monitor.name = map.value(QStringLiteral("name"), monitor.id).toString();
    monitor.x = map.value(QStringLiteral("x")).toInt();
    monitor.y = map.value(QStringLiteral("y")).toInt();
    monitor.width = map.value(QStringLiteral("width"), 1920).toInt();
    monitor.height = map.value(QStringLiteral("height"), 1080).toInt();
    monitor.scale = map.value(QStringLiteral("scale"), 1.0).toDouble();
    monitor.rotation = map.value(QStringLiteral("rotation")).toInt();
    monitor.enabled = map.value(QStringLiteral("enabled"), true).toBool();
    monitor.primary = map.value(QStringLiteral("primary")).toBool();
    return monitor;
}
}

ValidationResult MonitorLayout::validate() const
{
    const auto enabled = std::count_if(monitors.cbegin(), monitors.cend(), [](const Monitor &monitor) {
        return monitor.enabled;
    });
    if (enabled < 1 || enabled > MaximumMonitors) {
        return {false, QStringLiteral("A layout must contain between 1 and %1 enabled monitors").arg(MaximumMonitors)};
    }

    QSet<QString> ids;
    int primaryCount = 0;
    QList<QRect> rectangles;
    QRect desktop;
    for (const auto &monitor : monitors) {
        if (monitor.id.trimmed().isEmpty()) {
            return {false, QStringLiteral("Every monitor needs a stable id")};
        }
        if (ids.contains(monitor.id)) {
            return {false, QStringLiteral("Duplicate monitor id: %1").arg(monitor.id)};
        }
        ids.insert(monitor.id);
        if (!monitor.enabled) {
            continue;
        }
        if (monitor.width < MinimumDimension || monitor.width > MaximumDimension
            || monitor.height < MinimumDimension || monitor.height > MaximumDimension) {
            return {false,
                    QStringLiteral("%1 has an unsupported size; each dimension must be %2-%3 pixels")
                        .arg(monitor.name)
                        .arg(MinimumDimension)
                        .arg(MaximumDimension)};
        }
        if (monitor.scale < 0.5 || monitor.scale > 4.0) {
            return {false, QStringLiteral("%1 has an unsupported scale").arg(monitor.name)};
        }
        if (monitor.rotation != 0 && monitor.rotation != 90 && monitor.rotation != 180 && monitor.rotation != 270) {
            return {false, QStringLiteral("%1 has an unsupported rotation").arg(monitor.name)};
        }
        primaryCount += monitor.primary ? 1 : 0;

        const QRect rectangle(monitor.x, monitor.y, monitor.width, monitor.height);
        for (const auto &other : std::as_const(rectangles)) {
            if (rectangle.intersects(other)) {
                return {false, QStringLiteral("%1 overlaps another enabled monitor").arg(monitor.name)};
            }
        }
        rectangles.append(rectangle);
        desktop = desktop.isNull() ? rectangle : desktop.united(rectangle);
    }

    if (primaryCount != 1) {
        return {false, QStringLiteral("Exactly one enabled monitor must be primary")};
    }
    if (desktop.width() > MaximumDesktopDimension || desktop.height() > MaximumDesktopDimension) {
        return {false, QStringLiteral("The combined desktop exceeds the RDP graphics limit")};
    }
    return {};
}

MonitorLayout MonitorLayout::normalized() const
{
    MonitorLayout result = *this;
    int minimumX = 0;
    int minimumY = 0;
    bool first = true;
    for (const auto &monitor : monitors) {
        if (!monitor.enabled) {
            continue;
        }
        minimumX = first ? monitor.x : std::min(minimumX, monitor.x);
        minimumY = first ? monitor.y : std::min(minimumY, monitor.y);
        first = false;
    }
    for (auto &monitor : result.monitors) {
        if (monitor.enabled) {
            monitor.x -= minimumX;
            monitor.y -= minimumY;
        }
    }
    return result;
}

QVariantList MonitorLayout::toVariantList() const
{
    QVariantList result;
    result.reserve(monitors.size());
    for (const auto &monitor : monitors) {
        result.append(monitorToMap(monitor));
    }
    return result;
}

QJsonObject MonitorLayout::toJson() const
{
    return {{QStringLiteral("monitors"), QJsonArray::fromVariantList(toVariantList())}};
}

MonitorLayout MonitorLayout::fromVariantList(const QVariantList &values, QString *error)
{
    MonitorLayout layout;
    layout.monitors.reserve(values.size());
    for (const auto &value : values) {
        if (!value.canConvert<QVariantMap>()) {
            if (error) {
                *error = QStringLiteral("Monitor entries must be dictionaries");
            }
            return {};
        }
        layout.monitors.append(monitorFromMap(value.toMap()));
    }
    const auto validation = layout.validate();
    if (!validation.valid && error) {
        *error = validation.error;
    }
    return layout;
}

MonitorLayout MonitorLayout::fromJson(const QJsonObject &object, QString *error)
{
    return fromVariantList(object.value(QStringLiteral("monitors")).toArray().toVariantList(), error);
}

MonitorLayout MonitorLayout::fallback()
{
    return {{{QStringLiteral("KHEADLESS-1"), QStringLiteral("KHeadless Display"), 0, 0, 1920, 1080, 1.0, 0, true, true}}};
}

