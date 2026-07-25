#include "rdpbackend.h"

#include <QDir>
#include <QProcessEnvironment>
#include <QSettings>
#include <QStandardPaths>

#include <algorithm>

using namespace KHeadless;

QString UnavailableRdpBackend::name() const
{
    return QStringLiteral("not-built");
}

bool UnavailableRdpBackend::available() const
{
    return false;
}

bool UnavailableRdpBackend::running() const
{
    return false;
}

bool UnavailableRdpBackend::start(const ServerSettings &, const MonitorLayout &, const QList<RdpCredential> &, QString *error)
{
    const auto message =
        QStringLiteral("The embedded KRdp backend was not built. Configure with -DKHEADLESS_BUILD_KRDP=ON after applying the KRdp patch series.");
    if (error) {
        *error = message;
    }
    Q_EMIT diagnostic(message);
    return false;
}

void UnavailableRdpBackend::stop()
{
}

bool UnavailableRdpBackend::prepareLayout(const MonitorLayout &, QString *error)
{
    if (error) {
        *error = QStringLiteral("The embedded KRdp backend is unavailable");
    }
    return false;
}

bool UnavailableRdpBackend::updateLayout(const MonitorLayout &, QString *error)
{
    if (error) {
        *error = QStringLiteral("The embedded KRdp backend is unavailable");
    }
    return false;
}

bool UnavailableRdpBackend::acceptClientLayout(const QString &, const MonitorLayout &, QString *error)
{
    if (error) {
        *error = QStringLiteral("The embedded KRdp backend is unavailable");
    }
    return false;
}

ProcessRdpBackend::ProcessRdpBackend(QObject *parent)
    : RdpBackend(parent)
{
    connect(&m_process, &QProcess::stateChanged, this, [this](QProcess::ProcessState) {
        Q_EMIT runningChanged();
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        Q_EMIT diagnostic(m_process.errorString());
    });
}

QString ProcessRdpBackend::name() const
{
    return QStringLiteral("krdp-process-compat");
}

bool ProcessRdpBackend::available() const
{
    return !QStandardPaths::findExecutable(QStringLiteral("krdpserver")).isEmpty();
}

bool ProcessRdpBackend::running() const
{
    return m_process.state() != QProcess::NotRunning;
}

bool ProcessRdpBackend::start(const ServerSettings &settings,
                              const MonitorLayout &layout,
                              const QList<RdpCredential> &credentials,
                              QString *error)
{
    if (!available()) {
        if (error) {
            *error = QStringLiteral("krdpserver was not found in PATH");
        }
        return false;
    }
    if (credentials.isEmpty()) {
        if (error) {
            *error = QStringLiteral("At least one RDP user is required");
        }
        return false;
    }
    const auto enabledCount = std::count_if(layout.monitors.cbegin(), layout.monitors.cend(), [](const Monitor &monitor) {
        return monitor.enabled;
    });
    if (enabledCount != 1) {
        if (error) {
            *error = QStringLiteral("The stock KRdp compatibility backend supports one monitor; apply the downstream KRdp patch series for multi-monitor");
        }
        return false;
    }

    m_settings = settings;
    m_layout = layout;
    m_credentials = credentials;
    return launch(error);
}

void ProcessRdpBackend::stop()
{
    if (!running()) {
        return;
    }
    m_process.terminate();
    if (!m_process.waitForFinished(5'000)) {
        m_process.kill();
        m_process.waitForFinished();
    }
}

bool ProcessRdpBackend::prepareLayout(const MonitorLayout &layout, QString *error)
{
    const auto enabledCount = std::count_if(layout.monitors.cbegin(), layout.monitors.cend(), [](const Monitor &monitor) {
        return monitor.enabled;
    });
    if (enabledCount != 1) {
        if (error) {
            *error = QStringLiteral("The stock KRdp compatibility backend supports one monitor");
        }
        return false;
    }
    return true;
}

bool ProcessRdpBackend::updateLayout(const MonitorLayout &layout, QString *error)
{
    const bool wasRunning = running();
    if (wasRunning) {
        stop();
    }
    m_layout = layout;
    if (!wasRunning) {
        return true;
    }
    Q_EMIT diagnostic(QStringLiteral("Restarting the stock KRdp compatibility backend for a layout change"));
    return start(m_settings, m_layout, m_credentials, error);
}

bool ProcessRdpBackend::acceptClientLayout(const QString &, const MonitorLayout &, QString *error)
{
    if (error) {
        *error = QStringLiteral("Stock KRdp does not expose client Display Control events to KHeadless");
    }
    return false;
}

bool ProcessRdpBackend::launch(QString *error)
{
    const auto monitor = *std::find_if(m_layout.monitors.cbegin(), m_layout.monitors.cend(), [](const Monitor &candidate) {
        return candidate.enabled;
    });

    const auto dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + QStringLiteral("/krdp-compat");
    const auto configPath = dataPath + QStringLiteral("/config");
    QDir().mkpath(configPath);
    QSettings krdpSettings(configPath + QStringLiteral("/krdpserverrc"), QSettings::IniFormat);
    krdpSettings.beginGroup(QStringLiteral("General"));
    QStringList users;
    users.reserve(m_credentials.size());
    for (const auto &credential : std::as_const(m_credentials)) {
        users.append(credential.username);
    }
    krdpSettings.setValue(QStringLiteral("Users"), users);
    krdpSettings.setValue(QStringLiteral("ListenPort"), m_settings.port);
    krdpSettings.setValue(QStringLiteral("Certificate"), m_settings.certificate);
    krdpSettings.setValue(QStringLiteral("CertificateKey"), m_settings.certificateKey);
    krdpSettings.setValue(QStringLiteral("Quality"), m_settings.quality);
    krdpSettings.endGroup();
    krdpSettings.sync();

    auto environment = QProcessEnvironment::systemEnvironment();
    environment.insert(QStringLiteral("XDG_CONFIG_HOME"), configPath);
    m_process.setProcessEnvironment(environment);
    m_process.setProgram(QStandardPaths::findExecutable(QStringLiteral("krdpserver")));
    m_process.setArguments({
        QStringLiteral("--plasma"),
        QStringLiteral("--address"),
        m_settings.address,
        QStringLiteral("--port"),
        QString::number(m_settings.port),
        QStringLiteral("--certificate"),
        m_settings.certificate,
        QStringLiteral("--certificate-key"),
        m_settings.certificateKey,
        QStringLiteral("--quality"),
        QString::number(m_settings.quality),
        QStringLiteral("--virtual-monitor"),
        QStringLiteral("%1x%2@%3").arg(monitor.width).arg(monitor.height).arg(monitor.scale),
    });
    m_process.start();
    if (!m_process.waitForStarted(10'000)) {
        if (error) {
            *error = m_process.errorString();
        }
        return false;
    }
    return true;
}
