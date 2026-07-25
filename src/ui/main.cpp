#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlExtensionPlugin>
#include <QQuickStyle>

Q_IMPORT_QML_PLUGIN(org_kde_kheadlessPlugin)

int main(int argc, char **argv)
{
    QGuiApplication application(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("kheadless-settings"));
    QGuiApplication::setApplicationDisplayName(QStringLiteral("KHeadless"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("kde.org"));
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/org/kde/kheadless/settings/qml/Main.qml")));
    if (engine.rootObjects().isEmpty()) {
        return 1;
    }
    return application.exec();
}
