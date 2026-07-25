#include "layoutcontroller.h"

#include "outputbackend.h"

using namespace KHeadless;

LayoutController::LayoutController(std::unique_ptr<OutputBackend> backend, QObject *parent)
    : QObject(parent)
    , m_backend(std::move(backend))
{
    m_revertTimer.setSingleShot(true);
    m_revertTimer.setInterval(15'000);
    connect(&m_revertTimer, &QTimer::timeout, this, [this]() {
        QString error;
        revert(&error);
    });
}

MonitorLayout LayoutController::layout() const
{
    return m_layout;
}

MonitorLayout LayoutController::pendingLayout() const
{
    return m_revertTimer.isActive() ? m_layout : MonitorLayout{};
}

MonitorLayout LayoutController::previousLayout() const
{
    return m_previousLayout;
}

LayoutController::Mode LayoutController::mode() const
{
    return m_mode;
}

QString LayoutController::backendName() const
{
    return m_backend->name();
}

bool LayoutController::backendAvailable() const
{
    return m_backend->available();
}

bool LayoutController::hasPendingConfirmation() const
{
    return m_revertTimer.isActive();
}

int LayoutController::confirmationSecondsRemaining() const
{
    return qMax(0, (m_revertTimer.remainingTime() + 999) / 1000);
}

bool LayoutController::apply(const MonitorLayout &layout, bool requireConfirmation, QString *error)
{
    const auto validation = layout.validate();
    if (!validation.valid) {
        if (error) {
            *error = validation.error;
        }
        Q_EMIT applyFailed(validation.error);
        return false;
    }

    QString backendError;
    if (!m_backend->apply(layout.normalized(), &backendError)) {
        if (error) {
            *error = backendError;
        }
        Q_EMIT applyFailed(backendError);
        return false;
    }

    m_previousLayout = m_layout;
    m_layout = layout;
    if (requireConfirmation) {
        m_revertTimer.start();
    } else {
        m_revertTimer.stop();
        m_previousLayout = m_layout;
    }
    Q_EMIT layoutChanged();
    Q_EMIT confirmationChanged();
    return true;
}

bool LayoutController::setInitialLayout(const MonitorLayout &layout, QString *error)
{
    const auto validation = layout.validate();
    if (!validation.valid) {
        if (error) {
            *error = validation.error;
        }
        return false;
    }
    if (m_revertTimer.isActive()) {
        if (error) {
            *error = QStringLiteral("Cannot replace the initial layout while confirmation is pending");
        }
        return false;
    }
    m_layout = layout;
    m_previousLayout = layout;
    Q_EMIT layoutChanged();
    return true;
}

bool LayoutController::confirm()
{
    if (!m_revertTimer.isActive()) {
        return false;
    }
    m_revertTimer.stop();
    m_previousLayout = m_layout;
    Q_EMIT confirmationChanged();
    return true;
}

bool LayoutController::revert(QString *error)
{
    if (!m_revertTimer.isActive()) {
        return false;
    }
    m_revertTimer.stop();
    Q_EMIT layoutReverting(m_previousLayout);
    QString backendError;
    if (!m_backend->apply(m_previousLayout.normalized(), &backendError)) {
        if (error) {
            *error = backendError;
        }
        Q_EMIT applyFailed(backendError);
        return false;
    }
    m_layout = m_previousLayout;
    Q_EMIT layoutChanged();
    Q_EMIT layoutReverted(m_layout);
    Q_EMIT confirmationChanged();
    return true;
}

void LayoutController::setMode(Mode mode)
{
    if (m_mode == mode) {
        return;
    }
    m_mode = mode;
    Q_EMIT modeChanged();
}
