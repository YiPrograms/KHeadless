#include "service.h"

#include "kscreenbackend.h"
#include "outputbackend.h"
#include "rdpbackend.h"
#ifdef KHEADLESS_HAVE_KRDP
#include "embeddedrdpbackend.h"
#endif

#include <QDBusConnection>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QHostAddress>
#include <QProcess>
#include <QStandardPaths>

#include <qt6keychain/keychain.h>

using namespace KHeadless;

namespace
{
bool writeKrdpSecret(const QString &username, const QString &password, QString *error)
{
    QKeychain::WritePasswordJob job(QStringLiteral("KRDP"));
    job.setKey(username);
    job.setTextData(password);
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error() != QKeychain::Error::NoError) {
        if (error) {
            *error = job.errorString();
        }
        return false;
    }
    return true;
}

bool deleteKrdpSecret(const QString &username, QString *error)
{
    QKeychain::DeletePasswordJob job(QStringLiteral("KRDP"));
    job.setKey(username);
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error() != QKeychain::Error::NoError && job.error() != QKeychain::Error::EntryNotFound) {
        if (error) {
            *error = job.errorString();
        }
        return false;
    }
    return true;
}

bool readKrdpSecret(const QString &username, QString *password, QString *error)
{
    QKeychain::ReadPasswordJob job(QStringLiteral("KRDP"));
    job.setKey(username);
    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
    if (job.error() != QKeychain::Error::NoError) {
        if (error) {
            *error = QStringLiteral("Unable to read the RDP credential for %1: %2").arg(username, job.errorString());
        }
        return false;
    }
    *password = job.textData();
    return true;
}
}

Service::Service(QObject *parent)
    : QObject(parent)
    , m_layout(std::make_unique<LayoutController>(
          qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY")
              ? std::unique_ptr<OutputBackend>(std::make_unique<MemoryOutputBackend>())
              : std::unique_ptr<OutputBackend>(std::make_unique<KScreenBackend>())))
#ifdef KHEADLESS_HAVE_KRDP
    , m_rdp(std::make_unique<EmbeddedRdpBackend>())
#else
    , m_rdp(std::make_unique<ProcessRdpBackend>())
#endif
{
    connect(m_layout.get(), &LayoutController::layoutChanged, this, [this]() {
        Q_EMIT layoutChanged();
    });
    connect(m_layout.get(), &LayoutController::modeChanged, this, [this]() {
        m_settings.setMode(mode());
        persist();
        Q_EMIT modeChanged();
    });
    connect(m_layout.get(), &LayoutController::confirmationChanged, this, [this]() {
        if (!m_layout->hasPendingConfirmation()) {
            m_settings.setLayout(m_layout->layout());
            persist();
        }
        Q_EMIT confirmationChanged();
    });
    connect(m_layout.get(), &LayoutController::applyFailed, this, &Service::reportError);
    connect(m_layout.get(), &LayoutController::layoutReverting, this, [this](const MonitorLayout &layout) {
        if (!running()) {
            return;
        }
        QString error;
        if (!m_rdp->prepareLayout(layout, &error)) {
            reportError(error);
        }
    });
    connect(m_layout.get(), &LayoutController::layoutReverted, this, [this](const MonitorLayout &layout) {
        if (!running()) {
            return;
        }
        QString error;
        if (!m_rdp->updateLayout(layout, &error)) {
            reportError(error);
        }
    });
    connect(&m_connections, &ConnectionRegistry::changed, this, &Service::connectionsChanged);
    connect(&m_connections, &ConnectionRegistry::controllerChanged, this, [this](const QString &) {
        Q_EMIT connectionsChanged();
    });
    connect(m_rdp.get(), &RdpBackend::runningChanged, this, &Service::runningChanged);
    connect(m_rdp.get(), &RdpBackend::diagnostic, this, &Service::reportError);
    connect(m_rdp.get(), &RdpBackend::connectionAuthenticated, this,
            [this](const QString &id, const QString &username, const QString &peer, bool readOnly) {
                m_connections.add(username, peer, readOnly, id);
                m_rdp->setController(m_connections.controllerId());
            });
    connect(m_rdp.get(), &RdpBackend::connectionClosed, this, [this](const QString &id) {
        m_connections.remove(id);
        m_rdp->setController(m_connections.controllerId());
    });
    connect(m_rdp.get(), &RdpBackend::monitorLayoutRequested, this,
            [this](const QString &id, const MonitorLayout &layout) {
                if (id != m_connections.controllerId() || m_layout->mode() != LayoutController::Mode::FollowClient) {
                    m_rdp->rejectClientLayout(id);
                    return;
                }
                QString error;
                if (applyLayoutTransaction(layout, false, id, &error)) {
                    m_connections.setRequestedLayout(id, layout);
                } else {
                    m_rdp->rejectClientLayout(id);
                    reportError(error);
                }
            });
}

Service::~Service() = default;

bool Service::initialize(QString *error)
{
    QString loadError;
    if (!m_settings.load(&loadError) || !m_credentials.load(&loadError)) {
        if (error) {
            *error = loadError;
        }
        return false;
    }

    const auto requestedMode = m_settings.mode();
    if (requestedMode == QLatin1StringView("manual")) {
        m_layout->setMode(LayoutController::Mode::Manual);
    } else if (requestedMode == QLatin1StringView("profile")) {
        m_layout->setMode(LayoutController::Mode::Profile);
    }
    QString layoutError;
    // KRdp owns the virtual-output lifecycle. Retain the desired topology now
    // and apply it only after Start() has created the matching KWin outputs.
    if (!m_layout->setInitialLayout(m_settings.layout(), &layoutError)) {
        if (error) {
            *error = layoutError;
        }
        return false;
    }
    return true;
}

QString Service::version() const { return QStringLiteral(KHEADLESS_VERSION); }
bool Service::running() const { return m_rdp->running(); }
QString Service::controller() const { return m_connections.controllerId(); }

QString Service::mode() const
{
    switch (m_layout->mode()) {
    case LayoutController::Mode::Manual:
        return QStringLiteral("manual");
    case LayoutController::Mode::Profile:
        return QStringLiteral("profile");
    case LayoutController::Mode::FollowClient:
        return QStringLiteral("follow-client");
    }
    return {};
}

QVariantMap Service::Status() const
{
    const auto server = m_settings.server();
    return {
        {QStringLiteral("version"), version()},
        {QStringLiteral("running"), running()},
        {QStringLiteral("mode"), mode()},
        {QStringLiteral("controller"), controller()},
        {QStringLiteral("connectionCount"), m_connections.connections().size()},
        {QStringLiteral("listenAddress"), server.address},
        {QStringLiteral("listenPort"), server.port},
        {QStringLiteral("rdpBackend"), m_rdp->name()},
        {QStringLiteral("outputBackend"), m_layout->backendName()},
        {QStringLiteral("confirmationPending"), m_layout->hasPendingConfirmation()},
        {QStringLiteral("confirmationSeconds"), m_layout->confirmationSecondsRemaining()},
    };
}

QVariantMap Service::ServerConfiguration() const
{
    const auto server = m_settings.server();
    return {
        {QStringLiteral("address"), server.address},
        {QStringLiteral("port"), server.port},
        {QStringLiteral("certificate"), server.certificate},
        {QStringLiteral("certificateKey"), server.certificateKey},
        {QStringLiteral("quality"), server.quality},
        {QStringLiteral("audio"), server.audio},
        {QStringLiteral("clipboard"), server.clipboard},
    };
}

ObjectList Service::Monitors() const
{
    return ObjectList::fromVariantList(m_layout->layout().toVariantList());
}

ObjectList Service::Connections() const
{
    return ObjectList::fromVariantList(m_connections.toVariantList());
}
QStringList Service::Users() const { return m_credentials.users(); }

QVariantMap Service::Diagnostics() const
{
    const auto runtime = qEnvironmentVariable("XDG_RUNTIME_DIR");
    const auto wayland = qEnvironmentVariable("WAYLAND_DISPLAY");
    const auto pipewire = qEnvironmentVariable("PIPEWIRE_REMOTE", QStringLiteral("pipewire-0"));
    const bool compatibilityBackend = m_rdp->name() == QLatin1StringView("krdp-process-compat");
    const bool embeddedBackend = m_rdp->name() == QLatin1StringView("krdp-embedded");
    return {
        {QStringLiteral("lastError"), m_lastError},
        {QStringLiteral("rdpBackend"), m_rdp->name()},
        {QStringLiteral("rdpBackendAvailable"), m_rdp->available()},
        {QStringLiteral("multiMonitorTransport"), !compatibilityBackend && m_rdp->available()},
        {QStringLiteral("dynamicResolutionTransport"), !compatibilityBackend && m_rdp->available()},
        {QStringLiteral("perMonitorSurfaces"), embeddedBackend && m_rdp->available()},
        {QStringLiteral("audioPlaybackTransport"), embeddedBackend && m_rdp->available()
             && !QStandardPaths::findExecutable(QStringLiteral("pw-cat")).isEmpty()},
        {QStringLiteral("outputBackend"), m_layout->backendName()},
        {QStringLiteral("outputBackendAvailable"), m_layout->backendAvailable()},
        {QStringLiteral("xdgRuntimeDir"), runtime},
        {QStringLiteral("waylandDisplay"), wayland},
        {QStringLiteral("waylandSocketExists"), !runtime.isEmpty() && !wayland.isEmpty() && QFile::exists(runtime + QLatin1Char('/') + wayland)},
        {QStringLiteral("pipewireRemote"), pipewire},
        {QStringLiteral("pipewireSocketExists"), !runtime.isEmpty() && QFile::exists(runtime + QLatin1Char('/') + pipewire)},
        {QStringLiteral("sessionBusConnected"), QDBusConnection::sessionBus().isConnected()},
        {QStringLiteral("configPath"), m_settings.path()},
        {QStringLiteral("credentialPath"), m_credentials.path()},
    };
}

QStringList Service::Profiles() const
{
    auto profiles = m_settings.profiles().keys();
    profiles.sort();
    return profiles;
}

bool Service::ApplyLayout(const ObjectList &monitors, bool temporary)
{
    QString error;
    const auto layout = MonitorLayout::fromVariantList(monitors.toVariantList(), &error);
    if (!error.isEmpty() || !applyLayoutTransaction(layout, temporary, {}, &error)) {
        reportError(error);
        return false;
    }
    return true;
}

bool Service::ConfirmLayout()
{
    return m_layout->confirm();
}

bool Service::RevertLayout()
{
    QString error;
    if (!m_layout->revert(&error)) {
        if (!error.isEmpty()) {
            reportError(error);
        }
        return false;
    }
    return true;
}

bool Service::SetMode(const QString &mode)
{
    if (mode == QLatin1StringView("follow-client")) {
        m_layout->setMode(LayoutController::Mode::FollowClient);
    } else if (mode == QLatin1StringView("manual")) {
        m_layout->setMode(LayoutController::Mode::Manual);
    } else if (mode == QLatin1StringView("profile")) {
        m_layout->setMode(LayoutController::Mode::Profile);
    } else {
        reportError(QStringLiteral("Mode must be follow-client, manual, or profile"));
        return false;
    }
    return true;
}

bool Service::ConfigureServer(const QVariantMap &configuration)
{
    auto server = m_settings.server();
    const auto address = configuration.value(QStringLiteral("address"), server.address).toString();
    const auto port = configuration.value(QStringLiteral("port"), server.port).toInt();
    const auto quality = configuration.value(QStringLiteral("quality"), server.quality).toInt();
    const auto certificate = configuration.value(QStringLiteral("certificate"), server.certificate).toString();
    const auto certificateKey = configuration.value(QStringLiteral("certificateKey"), server.certificateKey).toString();
    if (QHostAddress(address).isNull()) {
        reportError(QStringLiteral("Listen address must be a valid IPv4 or IPv6 address"));
        return false;
    }
    if (port < 1 || port > 65535) {
        reportError(QStringLiteral("Listen port must be between 1 and 65535"));
        return false;
    }
    if (quality < 0 || quality > 100) {
        reportError(QStringLiteral("Video quality must be between 0 and 100"));
        return false;
    }
    if (certificate.isEmpty() != certificateKey.isEmpty()) {
        reportError(QStringLiteral("TLS certificate and key must be configured together"));
        return false;
    }
    if (!certificate.isEmpty() && (!QFile::exists(certificate) || !QFile::exists(certificateKey))) {
        reportError(QStringLiteral("The configured TLS certificate or private key does not exist"));
        return false;
    }

    const bool restart = running();
    if (restart) {
        m_rdp->stop();
    }
    server.address = address;
    server.port = static_cast<quint16>(port);
    server.quality = quality;
    server.certificate = certificate;
    server.certificateKey = certificateKey;
    server.audio = configuration.value(QStringLiteral("audio"), server.audio).toBool();
    server.clipboard = configuration.value(QStringLiteral("clipboard"), server.clipboard).toBool();
    m_settings.setServer(server);
    if (!persist()) {
        return false;
    }
    if (restart) {
        return Start();
    }
    return true;
}

bool Service::SaveProfile(const QString &name, const ObjectList &monitors)
{
    QString error;
    const auto layout = MonitorLayout::fromVariantList(monitors.toVariantList(), &error);
    if (!error.isEmpty() || !m_settings.saveProfile(name, layout, &error) || !persist()) {
        reportError(error.isEmpty() ? QStringLiteral("Unable to save profile") : error);
        return false;
    }
    Q_EMIT profilesChanged();
    return true;
}

bool Service::ApplyProfile(const QString &name, bool temporary)
{
    const auto profiles = m_settings.profiles();
    if (!profiles.contains(name)) {
        reportError(QStringLiteral("Unknown profile: %1").arg(name));
        return false;
    }
    m_layout->setMode(LayoutController::Mode::Profile);
    QString error;
    const auto layout = profiles.value(name);
    if (!applyLayoutTransaction(layout, temporary, {}, &error)) {
        reportError(error);
        return false;
    }
    return true;
}

bool Service::DeleteProfile(const QString &name)
{
    if (!m_settings.deleteProfile(name)) {
        reportError(QStringLiteral("Unknown profile: %1").arg(name));
        return false;
    }
    persist();
    Q_EMIT profilesChanged();
    return true;
}

bool Service::SetPassword(const QString &username, const QString &password)
{
    QString error;
    if (!m_credentials.setPassword(username, password, &error)) {
        reportError(error);
        return false;
    }
    const auto normalized = username.trimmed();
    if (!writeKrdpSecret(normalized, password, &error)) {
        m_credentials.removeUser(normalized);
        reportError(error);
        return false;
    }
    if (!m_credentials.save(&error)) {
        QString ignored;
        deleteKrdpSecret(normalized, &ignored);
        m_credentials.removeUser(normalized);
        reportError(error);
        return false;
    }
    Q_EMIT usersChanged();
    return true;
}

bool Service::DeleteUser(const QString &username)
{
    if (!m_credentials.users().contains(username)) {
        reportError(QStringLiteral("Unknown user: %1").arg(username));
        return false;
    }
    QString error;
    if (!deleteKrdpSecret(username, &error)) {
        reportError(error);
        return false;
    }
    m_credentials.removeUser(username);
    if (!m_credentials.save(&error)) {
        reportError(error);
        return false;
    }
    Q_EMIT usersChanged();
    return true;
}

bool Service::Start()
{
    if (running()) {
        return true;
    }
    QString error;
    if (m_credentials.users().isEmpty()) {
        reportError(QStringLiteral("Create at least one RDP user before starting the server"));
        return false;
    }
    if (!ensureCertificate(&error)) {
        reportError(error);
        return false;
    }
    QList<RdpCredential> credentials;
    for (const auto &username : m_credentials.users()) {
        QString password;
        if (!readKrdpSecret(username, &password, &error)) {
            reportError(error);
            return false;
        }
        credentials.append({username, password, false});
    }
    if (!m_rdp->start(m_settings.server(), m_layout->layout(), credentials, &error)) {
        reportError(error);
        return false;
    }
    if (!m_layout->apply(m_layout->layout(), false, &error)) {
        m_rdp->stop();
        reportError(error);
        return false;
    }
    Q_EMIT runningChanged();
    return true;
}

bool Service::Stop()
{
    m_rdp->stop();
    Q_EMIT runningChanged();
    return true;
}

void Service::reportError(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }
    m_lastError = message;
    Q_EMIT Error(message);
}

bool Service::ensureCertificate(QString *error)
{
    auto settings = m_settings.server();
    if (!settings.certificate.isEmpty() && !settings.certificateKey.isEmpty()
        && QFile::exists(settings.certificate) && QFile::exists(settings.certificateKey)) {
        return true;
    }

    const auto dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataPath);
    settings.certificate = dataPath + QStringLiteral("/server.crt");
    settings.certificateKey = dataPath + QStringLiteral("/server.key");

    QProcess openssl;
    openssl.start(QStringLiteral("openssl"),
                  {QStringLiteral("req"),
                   QStringLiteral("-x509"),
                   QStringLiteral("-newkey"),
                   QStringLiteral("rsa:3072"),
                   QStringLiteral("-nodes"),
                   QStringLiteral("-days"),
                   QStringLiteral("825"),
                   QStringLiteral("-subj"),
                   QStringLiteral("/CN=KHeadless"),
                   QStringLiteral("-keyout"),
                   settings.certificateKey,
                   QStringLiteral("-out"),
                   settings.certificate});
    if (!openssl.waitForFinished(30'000) || openssl.exitStatus() != QProcess::NormalExit || openssl.exitCode() != 0) {
        if (error) {
            *error = QStringLiteral("Unable to generate TLS certificate: %1").arg(QString::fromUtf8(openssl.readAllStandardError()));
        }
        return false;
    }
    QFile::setPermissions(settings.certificateKey, QFileDevice::ReadOwner | QFileDevice::WriteOwner);
    m_settings.setServer(settings);
    return persist();
}

bool Service::applyLayoutTransaction(const MonitorLayout &layout,
                                     bool temporary,
                                     const QString &clientConnectionId,
                                     QString *error)
{
    const auto previous = m_layout->layout();
    if (running() && !m_rdp->prepareLayout(layout, error)) {
        return false;
    }
    if (!m_layout->apply(layout, true, error)) {
        if (running()) {
            QString ignored;
            m_rdp->prepareLayout(previous, &ignored);
        }
        return false;
    }

    bool backendAccepted = true;
    if (running()) {
        backendAccepted = clientConnectionId.isEmpty()
            ? m_rdp->updateLayout(layout, error)
            : m_rdp->acceptClientLayout(clientConnectionId, layout, error);
    }
    if (!backendAccepted) {
        QString rollbackError;
        m_layout->revert(&rollbackError);
        if (!rollbackError.isEmpty() && error) {
            *error = QStringLiteral("%1; rollback also failed: %2").arg(*error, rollbackError);
        }
        return false;
    }

    if (!temporary) {
        m_layout->confirm();
    }
    return true;
}

bool Service::persist()
{
    QString error;
    if (!m_settings.save(&error)) {
        reportError(error);
        return false;
    }
    return true;
}
