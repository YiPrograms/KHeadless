#pragma once

#include "monitorlayout.h"

#include <QObject>
#include <QTimer>

#include <memory>

namespace KHeadless
{

class OutputBackend;

class LayoutController : public QObject
{
    Q_OBJECT
public:
    enum class Mode {
        FollowClient,
        Manual,
        Profile,
    };
    Q_ENUM(Mode)

    explicit LayoutController(std::unique_ptr<OutputBackend> backend, QObject *parent = nullptr);

    MonitorLayout layout() const;
    MonitorLayout pendingLayout() const;
    MonitorLayout previousLayout() const;
    Mode mode() const;
    QString backendName() const;
    bool backendAvailable() const;
    bool hasPendingConfirmation() const;
    int confirmationSecondsRemaining() const;

    bool apply(const MonitorLayout &layout, bool requireConfirmation, QString *error = nullptr);
    bool setInitialLayout(const MonitorLayout &layout, QString *error = nullptr);
    bool confirm();
    bool revert(QString *error = nullptr);
    void setMode(Mode mode);

Q_SIGNALS:
    void layoutChanged();
    void layoutReverting(const KHeadless::MonitorLayout &layout);
    void layoutReverted(const KHeadless::MonitorLayout &layout);
    void modeChanged();
    void confirmationChanged();
    void applyFailed(const QString &error);

private:
    std::unique_ptr<OutputBackend> m_backend;
    MonitorLayout m_layout = MonitorLayout::fallback();
    MonitorLayout m_previousLayout = MonitorLayout::fallback();
    Mode m_mode = Mode::FollowClient;
    QTimer m_revertTimer;
};

}
