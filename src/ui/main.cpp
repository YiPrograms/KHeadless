#include <QGuiApplication>
#include <QCommandLineParser>
#include <QQmlComponent>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickStyle>

Q_IMPORT_QML_PLUGIN(org_kde_kheadlessPlugin)

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("kheadless-settings"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("KHeadless"));
    QGuiApplication::setApplicationVersion(QStringLiteral(KHEADLESS_VERSION));
    QGuiApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Configure the KHeadless Plasma RDP workspace"));
    parser.addHelpOption();
    parser.addVersionOption();
    const QCommandLineOption checkOption(
        QStringLiteral("check"),
        QStringLiteral("Verify that the embedded QML interface loads, then exit"));
    parser.addOption(checkOption);
    parser.process(application);

    QQmlApplicationEngine engine;
    const QUrl mainUrl(QStringLiteral("qrc:/org/kde/kheadless/settings/qml/Main.qml"));
    if (parser.isSet(checkOption)) {
        QQmlComponent component(&engine, mainUrl);
        if (component.isError()) {
            for (const auto &error : component.errors()) {
                qCritical().noquote() << error.toString();
            }
            return 1;
        }
        return component.isReady() ? 0 : 1;
    }

    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return application.exec();
}
