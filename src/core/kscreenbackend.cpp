#include "kscreenbackend.h"

#include <KScreen/Config>
#include <KScreen/GetConfigOperation>
#include <KScreen/Mode>
#include <KScreen/Output>
#include <KScreen/SetConfigOperation>

#include <QEventLoop>
#include <QSet>
#include <QSize>

using namespace KHeadless;

KScreenBackend::KScreenBackend(QObject *parent)
    : OutputBackend(parent)
{
}

QString KScreenBackend::name() const
{
    return QStringLiteral("libkscreen");
}

bool KScreenBackend::available() const
{
    return !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
}

bool KScreenBackend::apply(const MonitorLayout &layout, QString *error)
{
    if (!available()) {
        if (error) {
            *error = QStringLiteral("WAYLAND_DISPLAY is not set");
        }
        return false;
    }

    KScreen::GetConfigOperation getConfig;
    if (!getConfig.exec()) {
        if (error) {
            *error = getConfig.errorString();
        }
        return false;
    }

    const auto normalized = layout.normalized();
    const auto config = getConfig.config();
    QSet<int> usedOutputs;
    for (const auto &requested : normalized.monitors) {
        KScreen::OutputPtr output;
        for (const auto &candidate : config->outputs()) {
            if (candidate->name() == requested.id
                || candidate->name() == requested.name
                || candidate->name() == QStringLiteral("Virtual-") + requested.id
                || candidate->name() == QStringLiteral("Virtual-") + requested.name) {
                output = candidate;
                break;
            }
        }
        if (!output) {
            // Older stock KRdp versions name their KWin virtual output after a
            // size expression. Restrict the compatibility fallback to virtual
            // connectors so a physical or nested output is never rearranged.
            for (const auto &candidate : config->outputs()) {
                if (candidate->isConnected() && candidate->type() == KScreen::Output::Unknown
                    && candidate->name().startsWith(QStringLiteral("Virtual-"))
                    && !usedOutputs.contains(candidate->id())) {
                    output = candidate;
                    break;
                }
            }
        }
        if (!output) {
            if (!requested.enabled) {
                continue;
            }
            if (error) {
                *error = QStringLiteral("KWin has not created virtual output %1").arg(requested.id);
            }
            return false;
        }
        usedOutputs.insert(output->id());

        output->setEnabled(requested.enabled);
        if (!requested.enabled) {
            continue;
        }
        output->setPos(QPoint(requested.x, requested.y));
        output->setScale(requested.scale);
        output->setPriority(requested.primary ? 1 : 2);
        switch (requested.rotation) {
        case 90:
            output->setRotation(KScreen::Output::Right);
            break;
        case 180:
            output->setRotation(KScreen::Output::Inverted);
            break;
        case 270:
            output->setRotation(KScreen::Output::Left);
            break;
        default:
            output->setRotation(KScreen::Output::None);
        }

        for (const auto &mode : output->modes()) {
            if (mode->size() == QSize(requested.width, requested.height)) {
                output->setCurrentModeId(mode->id());
                break;
            }
        }
    }

    KScreen::SetConfigOperation setConfig(config);
    if (!setConfig.exec()) {
        if (error) {
            *error = setConfig.errorString();
        }
        return false;
    }
    return true;
}
