#include "dbustypes.h"

#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDBusInterface>
#include <QDBusMetaType>
#include <QDBusReply>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTextStream>

#include <termios.h>
#include <unistd.h>

namespace
{
constexpr auto ServiceName = "org.kde.KHeadless1";
constexpr auto ObjectPath = "/org/kde/KHeadless1";
constexpr auto InterfaceName = "org.kde.KHeadless1";

void printJson(const QVariant &value)
{
    QJsonDocument document;
    if (value.metaType().id() == QMetaType::QVariantMap) {
        document = QJsonDocument::fromVariant(value.toMap());
    } else {
        document = QJsonDocument::fromVariant(value.toList());
    }
    QTextStream(stdout) << document.toJson(QJsonDocument::Indented);
}

QVariantList readLayout(const QString &path, QString *error)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        *error = file.errorString();
        return {};
    }
    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (document.isNull()) {
        *error = parseError.errorString();
        return {};
    }
    return document.object().value(QStringLiteral("monitors")).toArray().toVariantList();
}
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("kheadlessctl"));
    QCoreApplication::setApplicationVersion(QStringLiteral(KHEADLESS_VERSION));
    qDBusRegisterMetaType<KHeadless::ObjectList>();

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Manage a KHeadless Plasma RDP session"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("command"), QStringLiteral("status, server, server-config, monitors, connections, diagnostics, start, stop, mode, apply, confirm, revert, profiles, profile-save, profile-apply, profile-delete, users, passwd, user-delete"));
    parser.addPositionalArgument(QStringLiteral("arguments"), QStringLiteral("Arguments for the command"), QStringLiteral("[arguments...]"));
    parser.addOption({QStringLiteral("temporary"), QStringLiteral("Require confirmation and revert after 15 seconds")});
    parser.addOption({QStringLiteral("address"), QStringLiteral("RDP listen address"), QStringLiteral("address")});
    parser.addOption({QStringLiteral("port"), QStringLiteral("RDP listen port"), QStringLiteral("port")});
    parser.addOption({QStringLiteral("quality"), QStringLiteral("Video quality (0-100)"), QStringLiteral("quality")});
    parser.addOption({QStringLiteral("certificate"), QStringLiteral("TLS certificate path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("certificate-key"), QStringLiteral("TLS private key path"), QStringLiteral("path")});
    parser.addOption({QStringLiteral("audio"), QStringLiteral("Enable playback audio: true or false"), QStringLiteral("boolean")});
    parser.addOption({QStringLiteral("clipboard"), QStringLiteral("Enable clipboard: true or false"), QStringLiteral("boolean")});
    parser.process(app);

    const auto arguments = parser.positionalArguments();
    if (arguments.isEmpty()) {
        parser.showHelp(2);
    }

    QDBusInterface service(QString::fromLatin1(ServiceName),
                           QString::fromLatin1(ObjectPath),
                           QString::fromLatin1(InterfaceName),
                           QDBusConnection::sessionBus());
    if (!service.isValid()) {
        QTextStream(stderr) << "kheadlessd is unavailable: " << service.lastError().message() << '\n';
        return 1;
    }

    const auto command = arguments.first();
    const auto callMap = [&service](const QString &method) {
        QDBusReply<QVariantMap> reply = service.call(method);
        if (!reply.isValid()) {
            QTextStream(stderr) << reply.error().message() << '\n';
            return 1;
        }
        printJson(reply.value());
        return 0;
    };
    const auto callList = [&service](const QString &method) {
        QDBusReply<KHeadless::ObjectList> reply = service.call(method);
        if (!reply.isValid()) {
            QTextStream(stderr) << reply.error().message() << '\n';
            return 1;
        }
        printJson(reply.value().toVariantList());
        return 0;
    };
    const auto callBool = [&service](const QString &method, const QVariantList &values = {}) {
        QDBusMessage reply = service.callWithArgumentList(QDBus::Block, method, values);
        if (reply.type() == QDBusMessage::ErrorMessage || reply.arguments().isEmpty() || !reply.arguments().first().toBool()) {
            QTextStream(stderr) << (reply.type() == QDBusMessage::ErrorMessage ? reply.errorMessage() : QStringLiteral("Operation rejected")) << '\n';
            return 1;
        }
        return 0;
    };

    if (command == QLatin1StringView("status")) return callMap(QStringLiteral("Status"));
    if (command == QLatin1StringView("server")) return callMap(QStringLiteral("ServerConfiguration"));
    if (command == QLatin1StringView("server-config")) {
        QDBusReply<QVariantMap> reply = service.call(QStringLiteral("ServerConfiguration"));
        if (!reply.isValid()) {
            QTextStream(stderr) << reply.error().message() << '\n';
            return 1;
        }
        auto configuration = reply.value();
        const auto setString = [&parser, &configuration](const QString &option, const QString &key) {
            if (parser.isSet(option)) configuration.insert(key, parser.value(option));
        };
        setString(QStringLiteral("address"), QStringLiteral("address"));
        setString(QStringLiteral("certificate"), QStringLiteral("certificate"));
        setString(QStringLiteral("certificate-key"), QStringLiteral("certificateKey"));
        if (parser.isSet(QStringLiteral("port"))) configuration.insert(QStringLiteral("port"), parser.value(QStringLiteral("port")).toInt());
        if (parser.isSet(QStringLiteral("quality"))) configuration.insert(QStringLiteral("quality"), parser.value(QStringLiteral("quality")).toInt());
        if (parser.isSet(QStringLiteral("audio"))) configuration.insert(QStringLiteral("audio"), parser.value(QStringLiteral("audio")) == QLatin1StringView("true"));
        if (parser.isSet(QStringLiteral("clipboard"))) configuration.insert(QStringLiteral("clipboard"), parser.value(QStringLiteral("clipboard")) == QLatin1StringView("true"));
        return callBool(QStringLiteral("ConfigureServer"), {configuration});
    }
    if (command == QLatin1StringView("diagnostics")) return callMap(QStringLiteral("Diagnostics"));
    if (command == QLatin1StringView("monitors")) return callList(QStringLiteral("Monitors"));
    if (command == QLatin1StringView("connections")) return callList(QStringLiteral("Connections"));
    if (command == QLatin1StringView("start")) return callBool(QStringLiteral("Start"));
    if (command == QLatin1StringView("stop")) return callBool(QStringLiteral("Stop"));
    if (command == QLatin1StringView("confirm")) return callBool(QStringLiteral("ConfirmLayout"));
    if (command == QLatin1StringView("revert")) return callBool(QStringLiteral("RevertLayout"));
    if (command == QLatin1StringView("mode") && arguments.size() == 2) {
        return callBool(QStringLiteral("SetMode"), {arguments.at(1)});
    }
    if (command == QLatin1StringView("apply") && arguments.size() == 2) {
        QString error;
        const auto layout = readLayout(arguments.at(1), &error);
        if (!error.isEmpty()) {
            QTextStream(stderr) << error << '\n';
            return 1;
        }
        return callBool(QStringLiteral("ApplyLayout"),
                        {QVariant::fromValue(KHeadless::ObjectList::fromVariantList(layout)),
                         parser.isSet(QStringLiteral("temporary"))});
    }
    if (command == QLatin1StringView("profiles")) {
        QDBusReply<QStringList> reply = service.call(QStringLiteral("Profiles"));
        if (!reply.isValid()) return 1;
        QTextStream(stdout) << reply.value().join(QLatin1Char('\n')) << '\n';
        return 0;
    }
    if (command == QLatin1StringView("profile-save") && arguments.size() == 3) {
        QString error;
        const auto layout = readLayout(arguments.at(2), &error);
        if (!error.isEmpty()) {
            QTextStream(stderr) << error << '\n';
            return 1;
        }
        return callBool(QStringLiteral("SaveProfile"),
                        {arguments.at(1), QVariant::fromValue(KHeadless::ObjectList::fromVariantList(layout))});
    }
    if (command == QLatin1StringView("profile-apply") && arguments.size() == 2) {
        return callBool(QStringLiteral("ApplyProfile"), {arguments.at(1), parser.isSet(QStringLiteral("temporary"))});
    }
    if (command == QLatin1StringView("profile-delete") && arguments.size() == 2) {
        return callBool(QStringLiteral("DeleteProfile"), {arguments.at(1)});
    }
    if (command == QLatin1StringView("users")) {
        QDBusReply<QStringList> reply = service.call(QStringLiteral("Users"));
        if (!reply.isValid()) return 1;
        QTextStream(stdout) << reply.value().join(QLatin1Char('\n')) << '\n';
        return 0;
    }
    if (command == QLatin1StringView("passwd") && arguments.size() == 2) {
        QTextStream input(stdin);
        QTextStream(stderr) << "Password: " << Qt::flush;
        termios original{};
        const bool terminal = isatty(STDIN_FILENO) && tcgetattr(STDIN_FILENO, &original) == 0;
        if (terminal) {
            auto hidden = original;
            hidden.c_lflag &= static_cast<tcflag_t>(~ECHO);
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &hidden);
        }
        const auto password = input.readLine();
        if (terminal) {
            tcsetattr(STDIN_FILENO, TCSAFLUSH, &original);
            QTextStream(stderr) << '\n';
        }
        return callBool(QStringLiteral("SetPassword"), {arguments.at(1), password});
    }
    if (command == QLatin1StringView("user-delete") && arguments.size() == 2) {
        return callBool(QStringLiteral("DeleteUser"), {arguments.at(1)});
    }

    QTextStream(stderr) << "Invalid command or arguments. Run kheadlessctl --help.\n";
    return 2;
}
#include "dbustypes.h"
