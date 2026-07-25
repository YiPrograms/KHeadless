#include "settingsstore.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QSaveFile>
#include <QStandardPaths>

using namespace KHeadless;

SettingsStore::SettingsStore(QString path)
    : m_path(path.isEmpty()
                 ? QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/config.json")
                 : std::move(path))
{
}

bool SettingsStore::load(QString *error)
{
    QFile file(m_path);
    if (!file.exists()) {
        return true;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull()) {
        if (error) {
            *error = parseError.errorString();
        }
        return false;
    }
    const auto root = document.object();
    const auto server = root.value(QStringLiteral("server")).toObject();
    m_server.address = server.value(QStringLiteral("address")).toString(m_server.address);
    m_server.port = static_cast<quint16>(server.value(QStringLiteral("port")).toInt(m_server.port));
    m_server.certificate = server.value(QStringLiteral("certificate")).toString();
    m_server.certificateKey = server.value(QStringLiteral("certificateKey")).toString();
    m_server.quality = server.value(QStringLiteral("quality")).toInt(m_server.quality);
    m_server.audio = server.value(QStringLiteral("audio")).toBool(true);
    m_server.clipboard = server.value(QStringLiteral("clipboard")).toBool(true);
    m_mode = root.value(QStringLiteral("mode")).toString(m_mode);
    QString layoutError;
    const auto loadedLayout = MonitorLayout::fromJson(root.value(QStringLiteral("layout")).toObject(), &layoutError);
    if (layoutError.isEmpty()) {
        m_layout = loadedLayout;
    }
    const auto profiles = root.value(QStringLiteral("profiles")).toObject();
    for (auto i = profiles.begin(); i != profiles.end(); ++i) {
        QString profileError;
        const auto profile = MonitorLayout::fromJson(i.value().toObject(), &profileError);
        if (profileError.isEmpty()) {
            m_profiles.insert(i.key(), profile);
        }
    }
    return true;
}

bool SettingsStore::save(QString *error) const
{
    QDir().mkpath(QFileInfo(m_path).absolutePath());
    QSaveFile file(m_path);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    QJsonObject profiles;
    for (auto i = m_profiles.cbegin(); i != m_profiles.cend(); ++i) {
        profiles.insert(i.key(), i.value().toJson());
    }
    const QJsonObject root{
        {QStringLiteral("mode"), m_mode},
        {QStringLiteral("layout"), m_layout.toJson()},
        {QStringLiteral("profiles"), profiles},
        {QStringLiteral("server"),
         QJsonObject{
             {QStringLiteral("address"), m_server.address},
             {QStringLiteral("port"), m_server.port},
             {QStringLiteral("certificate"), m_server.certificate},
             {QStringLiteral("certificateKey"), m_server.certificateKey},
             {QStringLiteral("quality"), m_server.quality},
             {QStringLiteral("audio"), m_server.audio},
             {QStringLiteral("clipboard"), m_server.clipboard},
         }},
    };
    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        if (error) {
            *error = file.errorString();
        }
        return false;
    }
    return true;
}

QString SettingsStore::path() const { return m_path; }
ServerSettings SettingsStore::server() const { return m_server; }
void SettingsStore::setServer(const ServerSettings &server) { m_server = server; }
QString SettingsStore::mode() const { return m_mode; }
void SettingsStore::setMode(const QString &mode) { m_mode = mode; }
MonitorLayout SettingsStore::layout() const { return m_layout; }
void SettingsStore::setLayout(const MonitorLayout &layout) { m_layout = layout; }
QHash<QString, MonitorLayout> SettingsStore::profiles() const { return m_profiles; }

bool SettingsStore::saveProfile(const QString &name, const MonitorLayout &layout, QString *error)
{
    if (name.trimmed().isEmpty()) {
        if (error) {
            *error = QStringLiteral("Profile name cannot be empty");
        }
        return false;
    }
    const auto result = layout.validate();
    if (!result.valid) {
        if (error) {
            *error = result.error;
        }
        return false;
    }
    m_profiles.insert(name.trimmed(), layout);
    return true;
}

bool SettingsStore::deleteProfile(const QString &name)
{
    return m_profiles.remove(name) > 0;
}

