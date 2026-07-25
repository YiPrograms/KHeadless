#include "dashboardcontroller.h"
#include "dbustypes.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusMessage>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QTimer>

using namespace KHeadless;

namespace
{
constexpr auto ServiceName = "org.kde.KHeadless1";
constexpr auto ObjectPath = "/org/kde/KHeadless1";
constexpr auto InterfaceName = "org.kde.KHeadless1";
}

DashboardController::DashboardController(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<ObjectList>();
    reconnect();
    auto refreshTimer = new QTimer(this);
    refreshTimer->setInterval(2'000);
    connect(refreshTimer, &QTimer::timeout, this, &DashboardController::refresh);
    refreshTimer->start();
}

DashboardController::~DashboardController() = default;

bool DashboardController::available() const { return m_service && m_service->isValid(); }
bool DashboardController::running() const { return m_status.value(QStringLiteral("running")).toBool(); }
QString DashboardController::mode() const { return m_status.value(QStringLiteral("mode"), QStringLiteral("follow-client")).toString(); }
QString DashboardController::controller() const { return m_status.value(QStringLiteral("controller")).toString(); }
QVariantList DashboardController::monitors() const { return m_monitors; }
QVariantList DashboardController::connections() const { return m_connections; }
QVariantMap DashboardController::diagnostics() const { return m_diagnostics; }
QVariantMap DashboardController::serverConfiguration() const { return m_serverConfiguration; }
QStringList DashboardController::profiles() const { return m_profiles; }
QStringList DashboardController::users() const { return m_users; }
QString DashboardController::lastError() const { return m_lastError; }
bool DashboardController::confirmationPending() const { return m_status.value(QStringLiteral("confirmationPending")).toBool(); }
int DashboardController::confirmationSeconds() const { return m_status.value(QStringLiteral("confirmationSeconds")).toInt(); }

void DashboardController::reconnect()
{
    const bool wasAvailable = available();
    m_service = std::make_unique<QDBusInterface>(QString::fromLatin1(ServiceName),
                                                 QString::fromLatin1(ObjectPath),
                                                 QString::fromLatin1(InterfaceName),
                                                 QDBusConnection::sessionBus());
    if (available()) {
        auto bus = QDBusConnection::sessionBus();
        bus.connect(QString::fromLatin1(ServiceName), QString::fromLatin1(ObjectPath), QString::fromLatin1(InterfaceName),
                    QStringLiteral("layoutChanged"), this, SLOT(refresh()));
        bus.connect(QString::fromLatin1(ServiceName), QString::fromLatin1(ObjectPath), QString::fromLatin1(InterfaceName),
                    QStringLiteral("connectionsChanged"), this, SLOT(refresh()));
        bus.connect(QString::fromLatin1(ServiceName), QString::fromLatin1(ObjectPath), QString::fromLatin1(InterfaceName),
                    QStringLiteral("Error"), this, SLOT(refresh()));
        refresh();
    }
    if (wasAvailable != available()) {
        Q_EMIT availableChanged();
    }
}

void DashboardController::refresh()
{
    if (!available()) {
        reconnect();
        return;
    }

    const QDBusReply<QVariantMap> statusReply = m_service->call(QStringLiteral("Status"));
    const QDBusReply<ObjectList> monitorReply = m_service->call(QStringLiteral("Monitors"));
    const QDBusReply<ObjectList> connectionReply = m_service->call(QStringLiteral("Connections"));
    const QDBusReply<QVariantMap> diagnosticsReply = m_service->call(QStringLiteral("Diagnostics"));
    const QDBusReply<QVariantMap> serverReply = m_service->call(QStringLiteral("ServerConfiguration"));
    const QDBusReply<QStringList> profilesReply = m_service->call(QStringLiteral("Profiles"));
    const QDBusReply<QStringList> usersReply = m_service->call(QStringLiteral("Users"));
    if (!statusReply.isValid()) {
        setError(statusReply.error().message());
        return;
    }
    m_status = statusReply.value();
    m_monitors = monitorReply.value().toVariantList();
    m_connections = connectionReply.value().toVariantList();
    m_diagnostics = diagnosticsReply.value();
    m_serverConfiguration = serverReply.value();
    m_profiles = profilesReply.value();
    m_users = usersReply.value();
    setError(m_diagnostics.value(QStringLiteral("lastError")).toString());
    Q_EMIT statusChanged();
    Q_EMIT monitorsChanged();
    Q_EMIT connectionsChanged();
    Q_EMIT diagnosticsChanged();
    Q_EMIT serverConfigurationChanged();
    Q_EMIT profilesChanged();
    Q_EMIT usersChanged();
}

bool DashboardController::applyLayout(bool temporary)
{
    const auto result = callBool(QStringLiteral("ApplyLayout"),
                                 {QVariant::fromValue(ObjectList::fromVariantList(m_monitors)), temporary});
    refresh();
    return result;
}

bool DashboardController::confirmLayout() { return callBool(QStringLiteral("ConfirmLayout")); }
bool DashboardController::revertLayout() { const auto result = callBool(QStringLiteral("RevertLayout")); refresh(); return result; }
bool DashboardController::setMode(const QString &mode) { const auto result = callBool(QStringLiteral("SetMode"), {mode}); refresh(); return result; }

void DashboardController::updateServerSetting(const QString &key, const QVariant &value)
{
    m_serverConfiguration.insert(key, value);
    Q_EMIT serverConfigurationChanged();
}

bool DashboardController::applyServerSettings()
{
    const auto result = callBool(QStringLiteral("ConfigureServer"), {m_serverConfiguration});
    refresh();
    return result;
}

void DashboardController::updateMonitor(int index, const QString &key, const QVariant &value)
{
    if (index < 0 || index >= m_monitors.size()) {
        return;
    }
    auto monitor = m_monitors.at(index).toMap();
    monitor.insert(key, value);
    m_monitors[index] = monitor;
    Q_EMIT monitorsChanged();
}

void DashboardController::updateMonitorPosition(int index, int x, int y)
{
    updateMonitor(index, QStringLiteral("x"), x);
    updateMonitor(index, QStringLiteral("y"), y);
}

void DashboardController::addMonitor()
{
    if (m_monitors.size() >= 16) {
        setError(QStringLiteral("RDP supports at most 16 monitors"));
        return;
    }
    const auto index = m_monitors.size() + 1;
    int right = 0;
    for (const auto &entry : std::as_const(m_monitors)) {
        const auto monitor = entry.toMap();
        if (monitor.value(QStringLiteral("enabled")).toBool()) {
            right = qMax(right, monitor.value(QStringLiteral("x")).toInt() + monitor.value(QStringLiteral("width")).toInt());
        }
    }
    m_monitors.append(QVariantMap{
        {QStringLiteral("id"), QStringLiteral("KHEADLESS-%1").arg(index)},
        {QStringLiteral("name"), QStringLiteral("Display %1").arg(index)},
        {QStringLiteral("x"), right},
        {QStringLiteral("y"), 0},
        {QStringLiteral("width"), 1920},
        {QStringLiteral("height"), 1080},
        {QStringLiteral("scale"), 1.0},
        {QStringLiteral("rotation"), 0},
        {QStringLiteral("enabled"), true},
        {QStringLiteral("primary"), false},
    });
    Q_EMIT monitorsChanged();
}

void DashboardController::removeMonitor(int index)
{
    if (index < 0 || index >= m_monitors.size() || m_monitors.size() == 1) {
        return;
    }
    const bool wasPrimary = m_monitors.at(index).toMap().value(QStringLiteral("primary")).toBool();
    m_monitors.removeAt(index);
    if (wasPrimary && !m_monitors.isEmpty()) {
        makePrimary(0);
    }
    Q_EMIT monitorsChanged();
}

void DashboardController::makePrimary(int index)
{
    for (int i = 0; i < m_monitors.size(); ++i) {
        auto monitor = m_monitors.at(i).toMap();
        monitor.insert(QStringLiteral("primary"), i == index);
        m_monitors[i] = monitor;
    }
    Q_EMIT monitorsChanged();
}

bool DashboardController::saveProfile(const QString &name)
{
    const auto result = callBool(QStringLiteral("SaveProfile"),
                                 {name, QVariant::fromValue(ObjectList::fromVariantList(m_monitors))});
    refresh();
    return result;
}

bool DashboardController::applyProfile(const QString &name)
{
    const auto result = callBool(QStringLiteral("ApplyProfile"), {name, true});
    refresh();
    return result;
}

bool DashboardController::deleteProfile(const QString &name)
{
    const auto result = callBool(QStringLiteral("DeleteProfile"), {name});
    refresh();
    return result;
}

bool DashboardController::setPassword(const QString &username, const QString &password)
{
    const auto result = callBool(QStringLiteral("SetPassword"), {username, password});
    refresh();
    return result;
}

bool DashboardController::deleteUser(const QString &username)
{
    const auto result = callBool(QStringLiteral("DeleteUser"), {username});
    refresh();
    return result;
}

bool DashboardController::startServer() { const auto result = callBool(QStringLiteral("Start")); refresh(); return result; }
bool DashboardController::stopServer() { const auto result = callBool(QStringLiteral("Stop")); refresh(); return result; }

bool DashboardController::callBool(const QString &method, const QVariantList &arguments)
{
    if (!available()) {
        setError(QStringLiteral("kheadlessd is not running"));
        return false;
    }
    const auto reply = m_service->callWithArgumentList(QDBus::Block, method, arguments);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        setError(reply.errorMessage());
        return false;
    }
    if (reply.arguments().isEmpty() || !reply.arguments().first().toBool()) {
        refresh();
        if (m_lastError.isEmpty()) {
            setError(QStringLiteral("The daemon rejected the operation"));
        }
        return false;
    }
    return true;
}

void DashboardController::setError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }
    m_lastError = error;
    Q_EMIT errorChanged();
}
