#pragma once

#include "monitorlayout.h"
#include "settingsstore.h"

#include <QObject>
#include <QProcess>
#include <QStringList>

namespace KHeadless
{

struct RdpCredential {
    QString username;
    QString password;
    bool readOnly = false;
};

class RdpBackend : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    ~RdpBackend() override = default;

    virtual QString name() const = 0;
    virtual bool available() const = 0;
    virtual bool running() const = 0;
    virtual bool start(const ServerSettings &settings,
                       const MonitorLayout &layout,
                       const QList<RdpCredential> &credentials,
                       QString *error) = 0;
    virtual void stop() = 0;
    virtual bool prepareLayout(const MonitorLayout &layout, QString *error) = 0;
    virtual bool updateLayout(const MonitorLayout &layout, QString *error) = 0;
    virtual void setController(const QString &connectionId) = 0;
    virtual bool acceptClientLayout(const QString &connectionId,
                                    const MonitorLayout &layout,
                                    QString *error) = 0;
    virtual void rejectClientLayout(const QString &connectionId) = 0;

Q_SIGNALS:
    void runningChanged();
    void diagnostic(const QString &message);
    void connectionAuthenticated(const QString &connectionId,
                                 const QString &username,
                                 const QString &peerAddress,
                                 bool readOnly);
    void connectionClosed(const QString &connectionId);
    void monitorLayoutRequested(const QString &connectionId,
                                const KHeadless::MonitorLayout &layout);
};

class UnavailableRdpBackend final : public RdpBackend
{
    Q_OBJECT
public:
    using RdpBackend::RdpBackend;

    QString name() const override;
    bool available() const override;
    bool running() const override;
    bool start(const ServerSettings &, const MonitorLayout &, const QList<RdpCredential> &, QString *error) override;
    void stop() override;
    bool prepareLayout(const MonitorLayout &, QString *error) override;
    bool updateLayout(const MonitorLayout &, QString *error) override;
    void setController(const QString &) override {}
    bool acceptClientLayout(const QString &, const MonitorLayout &, QString *error) override;
    void rejectClientLayout(const QString &) override {}
};

class ProcessRdpBackend final : public RdpBackend
{
    Q_OBJECT
public:
    explicit ProcessRdpBackend(QObject *parent = nullptr);

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
    void setController(const QString &) override {}
    bool acceptClientLayout(const QString &, const MonitorLayout &, QString *error) override;
    void rejectClientLayout(const QString &) override {}

private:
    bool launch(QString *error);

    QProcess m_process;
    ServerSettings m_settings;
    MonitorLayout m_layout;
    QList<RdpCredential> m_credentials;
};

}
