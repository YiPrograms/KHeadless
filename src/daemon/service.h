#pragma once

#include "connectionregistry.h"
#include "credentialstore.h"
#include "dbustypes.h"
#include "layoutcontroller.h"
#include "settingsstore.h"

#include <QObject>
#include <QVariantList>
#include <QVariantMap>

#include <memory>

namespace KHeadless
{

class RdpBackend;

class Service final : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.KHeadless1")
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY modeChanged)
    Q_PROPERTY(QString controller READ controller NOTIFY connectionsChanged)

public:
    explicit Service(QObject *parent = nullptr);
    ~Service() override;

    bool initialize(QString *error = nullptr);

    QString version() const;
    bool running() const;
    QString mode() const;
    QString controller() const;

public Q_SLOTS:
    QVariantMap Status() const;
    QVariantMap ServerConfiguration() const;
    ObjectList Monitors() const;
    ObjectList Connections() const;
    QVariantMap Diagnostics() const;
    QStringList Profiles() const;
    QStringList Users() const;

    bool ApplyLayout(const ObjectList &monitors, bool temporary);
    bool ConfirmLayout();
    bool RevertLayout();
    bool SetMode(const QString &mode);
    bool ConfigureServer(const QVariantMap &configuration);
    bool SaveProfile(const QString &name, const ObjectList &monitors);
    bool ApplyProfile(const QString &name, bool temporary);
    bool DeleteProfile(const QString &name);
    bool SetPassword(const QString &username, const QString &password);
    bool DeleteUser(const QString &username);
    bool Start();
    bool Stop();

Q_SIGNALS:
    void runningChanged();
    void modeChanged();
    void layoutChanged();
    void connectionsChanged();
    void profilesChanged();
    void usersChanged();
    void confirmationChanged();
    void Error(const QString &message);

private:
    void reportError(const QString &message);
    bool ensureCertificate(QString *error);
    bool applyLayoutTransaction(const MonitorLayout &layout,
                                bool temporary,
                                const QString &clientConnectionId,
                                QString *error);
    bool persist();

    SettingsStore m_settings;
    CredentialStore m_credentials;
    std::unique_ptr<LayoutController> m_layout;
    std::unique_ptr<RdpBackend> m_rdp;
    ConnectionRegistry m_connections;
    QString m_lastError;
};

}
