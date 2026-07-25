#pragma once

#include "rdpbackend.h"

#include <memory>

namespace KHeadless
{

class EmbeddedRdpBackend final : public RdpBackend
{
    Q_OBJECT

public:
    explicit EmbeddedRdpBackend(QObject *parent = nullptr);
    ~EmbeddedRdpBackend() override;

    QString name() const override;
    bool available() const override;
    bool running() const override;
    bool start(const ServerSettings &settings,
               const MonitorLayout &layout,
               const QList<RdpCredential> &credentials,
               QString *error) override;
    void stop() override;
    bool prepareLayout(const MonitorLayout &layout, QString *error) override;
    bool updateLayout(const MonitorLayout &layout, QString *error) override;
    void setController(const QString &connectionId) override;
    bool acceptClientLayout(const QString &connectionId,
                            const MonitorLayout &layout,
                            QString *error) override;
    void rejectClientLayout(const QString &connectionId) override;

private:
    class Private;
    const std::unique_ptr<Private> d;
};

}
