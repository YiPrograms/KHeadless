#pragma once

#include "monitorlayout.h"

#include <QObject>

namespace KHeadless
{

class OutputBackend : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~OutputBackend() override = default;

    virtual QString name() const = 0;
    virtual bool available() const = 0;
    virtual bool apply(const MonitorLayout &layout, QString *error) = 0;
};

class MemoryOutputBackend final : public OutputBackend
{
    Q_OBJECT
public:
    using OutputBackend::OutputBackend;

    QString name() const override { return QStringLiteral("memory"); }
    bool available() const override { return true; }
    bool apply(const MonitorLayout &layout, QString *) override
    {
        m_layout = layout;
        return true;
    }
    MonitorLayout layout() const { return m_layout; }

private:
    MonitorLayout m_layout = MonitorLayout::fallback();
};

}

