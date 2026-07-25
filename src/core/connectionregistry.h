#pragma once

#include "monitorlayout.h"

#include <QDateTime>
#include <QObject>
#include <QVariantList>

namespace KHeadless
{

struct Connection {
    QString id;
    QString username;
    QString peerAddress;
    QDateTime connectedAt;
    bool controller = false;
    bool readOnly = false;
    MonitorLayout requestedLayout;
};

class ConnectionRegistry : public QObject
{
    Q_OBJECT
public:
    explicit ConnectionRegistry(QObject *parent = nullptr);

    QString add(const QString &username,
                const QString &peerAddress,
                bool readOnly = false,
                const QString &connectionId = {});
    bool remove(const QString &id);
    bool setRequestedLayout(const QString &id, const MonitorLayout &layout);
    QString controllerId() const;
    QList<Connection> connections() const;
    QVariantList toVariantList() const;

Q_SIGNALS:
    void changed();
    void controllerChanged(const QString &connectionId);

private:
    void electController();
    QList<Connection> m_connections;
};

}
