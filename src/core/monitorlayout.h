#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QVariantList>

namespace KHeadless
{

struct Monitor {
    QString id;
    QString name;
    int x = 0;
    int y = 0;
    int width = 1920;
    int height = 1080;
    qreal scale = 1.0;
    int rotation = 0;
    bool enabled = true;
    bool primary = false;

    bool operator==(const Monitor &) const = default;
};

struct ValidationResult {
    bool valid = true;
    QString error;
};

class MonitorLayout
{
public:
    static constexpr int MaximumMonitors = 16;
    static constexpr int MinimumDimension = 200;
    static constexpr int MaximumDimension = 8192;
    static constexpr int MaximumDesktopDimension = 32766;

    QList<Monitor> monitors;

    ValidationResult validate() const;
    MonitorLayout normalized() const;
    QVariantList toVariantList() const;
    QJsonObject toJson() const;

    static MonitorLayout fromVariantList(const QVariantList &values, QString *error = nullptr);
    static MonitorLayout fromJson(const QJsonObject &object, QString *error = nullptr);
    static MonitorLayout fallback();

    bool operator==(const MonitorLayout &) const = default;
};

}

Q_DECLARE_METATYPE(KHeadless::Monitor)
Q_DECLARE_METATYPE(KHeadless::MonitorLayout)

