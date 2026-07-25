#include "connectionregistry.h"

#include <QUuid>

using namespace KHeadless;

ConnectionRegistry::ConnectionRegistry(QObject *parent)
    : QObject(parent)
{
}

QString ConnectionRegistry::add(const QString &username,
                                const QString &peerAddress,
                                bool readOnly,
                                const QString &connectionId)
{
    for (auto &connection : m_connections) {
        connection.controller = false;
    }
    Connection connection;
    connection.id = connectionId.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : connectionId;
    connection.username = username;
    connection.peerAddress = peerAddress;
    connection.connectedAt = QDateTime::currentDateTimeUtc();
    connection.readOnly = readOnly;
    connection.controller = !readOnly;
    m_connections.append(connection);
    electController();
    Q_EMIT changed();
    return connection.id;
}

bool ConnectionRegistry::remove(const QString &id)
{
    const auto oldController = controllerId();
    const auto removed = m_connections.removeIf([&id](const Connection &connection) {
        return connection.id == id;
    });
    if (!removed) {
        return false;
    }
    electController();
    Q_EMIT changed();
    if (oldController != controllerId()) {
        Q_EMIT controllerChanged(controllerId());
    }
    return true;
}

bool ConnectionRegistry::setRequestedLayout(const QString &id, const MonitorLayout &layout)
{
    for (auto &connection : m_connections) {
        if (connection.id == id) {
            connection.requestedLayout = layout;
            Q_EMIT changed();
            return true;
        }
    }
    return false;
}

QString ConnectionRegistry::controllerId() const
{
    for (const auto &connection : m_connections) {
        if (connection.controller) {
            return connection.id;
        }
    }
    return {};
}

QList<Connection> ConnectionRegistry::connections() const
{
    return m_connections;
}

QVariantList ConnectionRegistry::toVariantList() const
{
    QVariantList result;
    for (const auto &connection : m_connections) {
        result.append(QVariantMap{
            {QStringLiteral("id"), connection.id},
            {QStringLiteral("username"), connection.username},
            {QStringLiteral("peerAddress"), connection.peerAddress},
            {QStringLiteral("connectedAt"), connection.connectedAt.toString(Qt::ISODate)},
            {QStringLiteral("controller"), connection.controller},
            {QStringLiteral("readOnly"), connection.readOnly},
            {QStringLiteral("requestedMonitors"), connection.requestedLayout.toVariantList()},
        });
    }
    return result;
}

void ConnectionRegistry::electController()
{
    bool found = false;
    for (auto i = m_connections.rbegin(); i != m_connections.rend(); ++i) {
        const bool selected = !found && !i->readOnly;
        i->controller = selected;
        found = found || selected;
    }
}
