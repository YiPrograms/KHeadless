#pragma once

#include "outputbackend.h"

namespace KHeadless
{

class KScreenBackend final : public OutputBackend
{
    Q_OBJECT
public:
    explicit KScreenBackend(QObject *parent = nullptr);

    QString name() const override;
    bool available() const override;
    bool apply(const MonitorLayout &layout, QString *error) override;
};

}

