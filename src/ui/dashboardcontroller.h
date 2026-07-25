#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

class QDBusInterface;

namespace KHeadless
{

class DashboardController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool running READ running NOTIFY statusChanged)
    Q_PROPERTY(QString mode READ mode NOTIFY statusChanged)
    Q_PROPERTY(QString controller READ controller NOTIFY statusChanged)
    Q_PROPERTY(QVariantList monitors READ monitors NOTIFY monitorsChanged)
    Q_PROPERTY(QVariantList connections READ connections NOTIFY connectionsChanged)
    Q_PROPERTY(QVariantMap diagnostics READ diagnostics NOTIFY diagnosticsChanged)
    Q_PROPERTY(QVariantMap serverConfiguration READ serverConfiguration NOTIFY serverConfigurationChanged)
    Q_PROPERTY(QStringList profiles READ profiles NOTIFY profilesChanged)
    Q_PROPERTY(QStringList users READ users NOTIFY usersChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY errorChanged)
    Q_PROPERTY(bool confirmationPending READ confirmationPending NOTIFY statusChanged)
    Q_PROPERTY(int confirmationSeconds READ confirmationSeconds NOTIFY statusChanged)

public:
    explicit DashboardController(QObject *parent = nullptr);
    ~DashboardController() override;

    bool available() const;
    bool running() const;
    QString mode() const;
    QString controller() const;
    QVariantList monitors() const;
    QVariantList connections() const;
    QVariantMap diagnostics() const;
    QVariantMap serverConfiguration() const;
    QStringList profiles() const;
    QStringList users() const;
    QString lastError() const;
    bool confirmationPending() const;
    int confirmationSeconds() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE bool applyLayout(bool temporary = true);
    Q_INVOKABLE bool confirmLayout();
    Q_INVOKABLE bool revertLayout();
    Q_INVOKABLE bool setMode(const QString &mode);
    Q_INVOKABLE void updateServerSetting(const QString &key, const QVariant &value);
    Q_INVOKABLE bool applyServerSettings();
    Q_INVOKABLE void updateMonitor(int index, const QString &key, const QVariant &value);
    Q_INVOKABLE void updateMonitorPosition(int index, int x, int y);
    Q_INVOKABLE void addMonitor();
    Q_INVOKABLE void removeMonitor(int index);
    Q_INVOKABLE void makePrimary(int index);
    Q_INVOKABLE bool saveProfile(const QString &name);
    Q_INVOKABLE bool applyProfile(const QString &name);
    Q_INVOKABLE bool deleteProfile(const QString &name);
    Q_INVOKABLE bool setPassword(const QString &username, const QString &password);
    Q_INVOKABLE bool deleteUser(const QString &username);
    Q_INVOKABLE bool startServer();
    Q_INVOKABLE bool stopServer();

Q_SIGNALS:
    void availableChanged();
    void statusChanged();
    void monitorsChanged();
    void connectionsChanged();
    void diagnosticsChanged();
    void serverConfigurationChanged();
    void profilesChanged();
    void usersChanged();
    void errorChanged();

private:
    bool callBool(const QString &method, const QVariantList &arguments = {});
    void setError(const QString &error);
    void reconnect();

    std::unique_ptr<QDBusInterface> m_service;
    QVariantMap m_status;
    QVariantList m_monitors;
    QVariantList m_connections;
    QVariantMap m_diagnostics;
    QVariantMap m_serverConfiguration;
    QStringList m_profiles;
    QStringList m_users;
    QString m_lastError;
};

}
