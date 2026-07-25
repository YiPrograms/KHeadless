#include "service.h"
#include "dbustypes.h"

#include <QCommandLineParser>
#ifdef KHEADLESS_HAVE_KRDP
#include <QGuiApplication>
#else
#include <QCoreApplication>
#endif
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMetaType>

using namespace KHeadless;

int main(int argc, char **argv)
{
#ifdef KHEADLESS_HAVE_KRDP
    QGuiApplication application(argc, argv);
#else
    QCoreApplication application(argc, argv);
#endif
    QCoreApplication::setApplicationName(QStringLiteral("kheadlessd"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KHEADLESS_VERSION));
    QCoreApplication::setOrganizationDomain(QStringLiteral("kde.org"));
#ifdef KHEADLESS_HAVE_KRDP
    QGuiApplication::setDesktopFileName(QStringLiteral("org.kde.kheadlessd"));
#endif

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("KHeadless Plasma RDP coordinator"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.process(application);

    auto bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCritical("Unable to connect to the session D-Bus");
        return 1;
    }

    qDBusRegisterMetaType<ObjectList>();

    Service service;
    QString error;
    if (!service.initialize(&error)) {
        qCritical().noquote() << error;
        return 1;
    }
    if (!bus.registerObject(QStringLiteral("/org/kde/KHeadless1"),
                            &service,
                            QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties)) {
        qCritical().noquote() << bus.lastError().message();
        return 1;
    }
    if (!bus.registerService(QStringLiteral("org.kde.KHeadless1"))) {
        qCritical().noquote() << bus.lastError().message();
        return 1;
    }
    return application.exec();
}
