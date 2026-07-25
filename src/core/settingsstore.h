#pragma once

#include "monitorlayout.h"

#include <QHash>
#include <QJsonObject>
#include <QString>

namespace KHeadless
{

struct ServerSettings {
    QString address = QStringLiteral("0.0.0.0");
    quint16 port = 3389;
    QString certificate;
    QString certificateKey;
    int quality = 80;
    bool audio = true;
    bool clipboard = true;
};

class SettingsStore
{
public:
    explicit SettingsStore(QString path = {});

    bool load(QString *error = nullptr);
    bool save(QString *error = nullptr) const;

    QString path() const;
    ServerSettings server() const;
    void setServer(const ServerSettings &server);
    QString mode() const;
    void setMode(const QString &mode);
    MonitorLayout layout() const;
    void setLayout(const MonitorLayout &layout);
    QHash<QString, MonitorLayout> profiles() const;
    bool saveProfile(const QString &name, const MonitorLayout &layout, QString *error = nullptr);
    bool deleteProfile(const QString &name);

private:
    QString m_path;
    ServerSettings m_server;
    QString m_mode = QStringLiteral("follow-client");
    MonitorLayout m_layout = MonitorLayout::fallback();
    QHash<QString, MonitorLayout> m_profiles;
};

}

