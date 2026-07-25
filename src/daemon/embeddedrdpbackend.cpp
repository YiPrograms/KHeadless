#include "embeddedrdpbackend.h"
#include "rdplayoutconversion.h"

#include <algorithm>
#include <map>

#include <QClipboard>
#include <QCoreApplication>
#include <QEventLoop>
#include <QGuiApplication>
#include <QHostAddress>
#include <QMimeData>
#include <QPointer>
#include <QProcess>
#include <QStandardPaths>
#include <QTimer>
#include <QUuid>

#include <KSystemClipboard>

#include <AudioStream.h>
#include <Clipboard.h>
#include <Cursor.h>
#include <DisplayControl.h>
#include <InputHandler.h>
#include <PlasmaScreencastV1Session.h>
#include <RdpConnection.h>
#include <Server.h>
#include <VideoStream.h>

using namespace KHeadless;

namespace {

bool outputShapeMatches(const Monitor &first, const Monitor &second) {
  return first.id == second.id && first.width == second.width &&
         first.height == second.height &&
         qFuzzyCompare(first.scale, second.scale);
}

} // namespace

class EmbeddedRdpBackend::Private {
public:
  struct VirtualOutput {
    Monitor monitor;
    std::unique_ptr<KRdp::PlasmaScreencastV1Session> session;
    bool ready = false;
    QString error;
  };

  struct Connection {
    QString id;
    QPointer<KRdp::RdpConnection> connection;
    std::unique_ptr<KRdp::PlasmaScreencastV1Session> capture;
    bool readOnly = false;
  };

  explicit Private(EmbeddedRdpBackend *q) : q(q) {
    QObject::connect(
        KSystemClipboard::instance(), &KSystemClipboard::changed, q,
        [this](QClipboard::Mode mode) {
          if (mode != QClipboard::Clipboard || !settings.clipboard) {
            return;
          }
          const auto *data = KSystemClipboard::instance()->mimeData(mode);
          if (!data || !data->hasText()) {
            return;
          }
          const auto text = data->text();
          if (text == clientClipboardEcho) {
            clientClipboardEcho.clear();
            return;
          }
          for (auto &[id, state] : connections) {
            if (state->connection &&
                state->connection->clipboard()->enabled()) {
              state->connection->clipboard()->setServerText(text);
            }
          }
        });
    QObject::connect(
        &audioCapture, &QProcess::readyReadStandardOutput, q, [this]() {
          audioRemainder.append(audioCapture.readAllStandardOutput());
          const auto bytes =
              audioRemainder.size() - (audioRemainder.size() % 4);
          if (bytes == 0) {
            return;
          }
          const auto samples = audioRemainder.first(bytes);
          audioRemainder.remove(0, bytes);
          for (auto &[id, state] : connections) {
            if (state->connection) {
              state->connection->audioStream()->sendSamples(samples);
            }
          }
        });
    QObject::connect(
        &audioCapture, &QProcess::readyReadStandardError, q, [this]() {
          const auto message =
              QString::fromUtf8(audioCapture.readAllStandardError()).trimmed();
          if (!message.isEmpty()) {
            Q_EMIT this->q->diagnostic(
                QStringLiteral("PipeWire audio capture: %1").arg(message));
          }
        });
    QObject::connect(&audioCapture, &QProcess::errorOccurred, q,
                     [this](QProcess::ProcessError) {
                       Q_EMIT this->q->diagnostic(
                           QStringLiteral("PipeWire audio capture failed: %1")
                               .arg(audioCapture.errorString()));
                     });
  }

  bool startAudio(QString *error) {
    if (!settings.audio) {
      return true;
    }
    const auto pwCat = QStandardPaths::findExecutable(QStringLiteral("pw-cat"));
    if (pwCat.isEmpty()) {
      if (error) {
        *error = QStringLiteral("Audio is enabled but pw-cat was not found");
      }
      return false;
    }
    audioCapture.setProgram(pwCat);
    audioCapture.setArguments({
        QStringLiteral("--record"),
        QStringLiteral("--raw"),
        QStringLiteral("--format"),
        QStringLiteral("s16"),
        QStringLiteral("--rate"),
        QStringLiteral("48000"),
        QStringLiteral("--channels"),
        QStringLiteral("2"),
        QStringLiteral("--channel-map"),
        QStringLiteral("Stereo"),
        QStringLiteral("--target"),
        QStringLiteral("@DEFAULT_AUDIO_SINK@"),
        QStringLiteral("-"),
    });
    audioCapture.setProcessChannelMode(QProcess::SeparateChannels);
    audioCapture.start(QIODevice::ReadOnly);
    if (!audioCapture.waitForStarted(5'000)) {
      if (error) {
        *error = QStringLiteral("Unable to start PipeWire playback capture: %1")
                     .arg(audioCapture.errorString());
      }
      return false;
    }
    return true;
  }

  void stopAudio() {
    audioRemainder.clear();
    if (audioCapture.state() == QProcess::NotRunning) {
      return;
    }
    audioCapture.terminate();
    if (!audioCapture.waitForFinished(2'000)) {
      audioCapture.kill();
      audioCapture.waitForFinished();
    }
  }

  int outputIndex(const QString &outputId) const {
    int index = 0;
    for (const auto &monitor : layout.monitors) {
      if (!monitor.enabled) {
        continue;
      }
      if (monitor.id == outputId) {
        return index;
      }
      ++index;
    }
    return -1;
  }

  void wireOutput(VirtualOutput &output, Connection &state) {
    if (!state.connection) {
      return;
    }
    auto *video = state.connection->videoStream();
    const auto outputId = output.monitor.id;
    QObject::connect(output.session.get(),
                     &KRdp::AbstractSession::frameReceived, video,
                     [this, outputId, video](const KRdp::VideoFrame &source) {
                       auto frame = source;
                       frame.outputIndex = outputIndex(outputId);
                       if (frame.outputIndex >= 0) {
                         video->queueFrame(frame);
                       }
                     });
    auto *connection = state.connection.data();
    QObject::connect(output.session.get(), &KRdp::AbstractSession::cursorUpdate,
                     video, [connection](const PipeWireCursor &cursor) {
                       if (!connection) {
                         return;
                       }
                       KRdp::Cursor::CursorUpdate update;
                       update.hotspot = cursor.hotspot;
                       update.image = cursor.texture;
                       connection->cursor()->update(update);
                     });
    if (video->enabled()) {
      output.session->requestStreamingEnable(video);
    }
  }

  void syncOutputStreaming(Connection &state) {
    if (!state.connection) {
      return;
    }
    auto *video = state.connection->videoStream();
    for (auto &[id, output] : outputs) {
      if (video->enabled()) {
        output->session->requestStreamingEnable(video);
      } else {
        output->session->requestStreamingDisable(video);
      }
    }
  }

  bool ensureVirtualOutputs(const MonitorLayout &layout, QString *error) {
    const auto validation = layout.validate();
    if (!validation.valid) {
      if (error) {
        *error = validation.error;
      }
      return false;
    }
    if (!server) {
      if (error) {
        *error = QStringLiteral("The embedded KRdp server is not initialized");
      }
      return false;
    }

    for (auto iterator = outputs.begin(); iterator != outputs.end();) {
      const auto requested = std::find_if(
          layout.monitors.cbegin(), layout.monitors.cend(),
          [&iterator](const Monitor &monitor) {
            return monitor.enabled && monitor.id == iterator->first;
          });
      if (requested == layout.monitors.cend() ||
          !outputShapeMatches(iterator->second->monitor, *requested)) {
        iterator = outputs.erase(iterator);
      } else {
        ++iterator;
      }
    }

    QList<VirtualOutput *> pending;
    for (const auto &monitor : layout.monitors) {
      if (!monitor.enabled || outputs.contains(monitor.id)) {
        continue;
      }

      auto output = std::make_unique<VirtualOutput>();
      output->monitor = monitor;
      output->session =
          std::make_unique<KRdp::PlasmaScreencastV1Session>(server.get());
      output->session->setVirtualMonitor({
          monitor.id,
          QSize(monitor.width, monitor.height),
          monitor.scale,
      });
      output->session->setVideoQuality(quint8(settings.quality));
      auto outputPointer = output.get();
      QObject::connect(output->session.get(), &KRdp::AbstractSession::started,
                       q, [outputPointer]() { outputPointer->ready = true; });
      QObject::connect(output->session.get(), &KRdp::AbstractSession::error, q,
                       [outputPointer]() {
                         outputPointer->error =
                             QStringLiteral("KWin rejected virtual output %1")
                                 .arg(outputPointer->monitor.id);
                       });
      outputs.emplace(monitor.id, std::move(output));
      for (auto &[id, connection] : connections) {
        wireOutput(*outputPointer, *connection);
      }
      pending.append(outputPointer);
    }

    if (!pending.isEmpty()) {
      QEventLoop loop;
      QTimer timer;
      timer.setSingleShot(true);
      timer.setInterval(10'000);
      QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
      for (auto *output : std::as_const(pending)) {
        QObject::connect(output->session.get(), &KRdp::AbstractSession::started,
                         &loop, [&loop, &pending]() {
                           const bool finished =
                               std::all_of(pending.cbegin(), pending.cend(),
                                           [](const VirtualOutput *candidate) {
                                             return candidate->ready ||
                                                    !candidate->error.isEmpty();
                                           });
                           if (finished) {
                             loop.quit();
                           }
                         });
        QObject::connect(output->session.get(), &KRdp::AbstractSession::error,
                         &loop, &QEventLoop::quit);
        output->session->start();
      }
      timer.start();
      const bool alreadyFinished = std::all_of(
          pending.cbegin(), pending.cend(), [](const VirtualOutput *candidate) {
            return candidate->ready || !candidate->error.isEmpty();
          });
      if (!alreadyFinished) {
        loop.exec();
      }
    }

    for (auto *output : std::as_const(pending)) {
      if (!output->ready) {
        if (error) {
          *error =
              output->error.isEmpty()
                  ? QStringLiteral("Timed out while creating virtual output %1")
                        .arg(output->monitor.id)
                  : output->error;
        }
        return false;
      }
    }
    return true;
  }

  void createCapture(Connection &state) {
    if (!state.connection) {
      return;
    }
    state.capture.reset();
    state.capture =
        std::make_unique<KRdp::PlasmaScreencastV1Session>(server.get());
    state.capture->setActiveStream(0);
    state.capture->setVideoQuality(quint8(settings.quality));

    auto *capture = state.capture.get();
    const QPointer<KRdp::RdpConnection> connection = state.connection;
    QObject::connect(
        capture, &KRdp::AbstractSession::error, q, [this, connection]() {
          Q_EMIT q->diagnostic(QStringLiteral("KWin workspace capture failed"));
          if (connection) {
            connection->close(
                KRdp::RdpConnection::CloseReason::VideoInitFailed);
          }
        });
    QObject::connect(connection->videoStream(),
                     &KRdp::VideoStream::enabledChanged, capture,
                     [this, id = state.id]() {
                       const auto iterator = connections.find(id);
                       if (iterator != connections.end()) {
                         syncOutputStreaming(*iterator->second);
                       }
                     });
    QObject::connect(connection->videoStream(),
                     &KRdp::VideoStream::requestedFrameRateChanged, capture,
                     [this, connection]() {
                       for (auto &[id, output] : outputs) {
                         output->session->setVideoFrameRate(
                             connection->videoStream()->requestedFrameRate());
                       }
                     });
    QObject::connect(connection->inputHandler(),
                     &KRdp::InputHandler::inputEvent, capture,
                     &KRdp::AbstractSession::sendEvent);
    capture->start();
  }

  void addConnection(KRdp::RdpConnection *connection) {
    auto state = std::make_unique<Connection>();
    state->id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    state->connection = connection;
    const auto id = state->id;
    connections.emplace(id, std::move(state));
    auto &stored = *connections.at(id);

    connection->setMonitorLayout(toKrdpLayout(layout));
    createCapture(stored);
    for (auto &[outputId, output] : outputs) {
      wireOutput(*output, stored);
    }
    syncOutputStreaming(stored);

    QObject::connect(
        connection, &KRdp::RdpConnection::authenticated, q,
        [this, id](const QString &username, const QString &peer,
                   bool readOnly) {
          const auto iterator = connections.find(id);
          if (iterator == connections.end()) {
            return;
          }
          iterator->second->readOnly = readOnly;
          Q_EMIT q->connectionAuthenticated(id, username, peer, readOnly);
        },
        Qt::QueuedConnection);
    QObject::connect(
        connection->displayControl(),
        &KRdp::DisplayControl::requestedMonitorLayoutChanged, q,
        [this, id](const KRdp::DisplayMonitorList &monitors) {
          Q_EMIT q->monitorLayoutRequested(id, fromKrdpLayout(monitors));
        },
        Qt::QueuedConnection);
    QObject::connect(
        connection->clipboard(), &KRdp::Clipboard::clientTextChanged, q,
        [this, id](const QString &text) {
          const auto iterator = connections.find(id);
          if (iterator == connections.end() || id != controllerId ||
              iterator->second->readOnly || !settings.clipboard ||
              !iterator->second->connection ||
              !iterator->second->connection->clipboard()->enabled()) {
            return;
          }
          clientClipboardEcho = text;
          auto *data = new QMimeData();
          data->setText(text);
          KSystemClipboard::instance()->setMimeData(data,
                                                    QClipboard::Clipboard);
          QTimer::singleShot(1'000, q, [this, text]() {
            if (clientClipboardEcho == text) {
              clientClipboardEcho.clear();
            }
          });
        });
    QObject::connect(
        connection, &KRdp::RdpConnection::stateChanged, q,
        [this, id](KRdp::RdpConnection::State state) {
          if (state != KRdp::RdpConnection::State::Closed) {
            return;
          }
          Q_EMIT q->connectionClosed(id);
          connections.erase(id);
        },
        Qt::QueuedConnection);
  }

  EmbeddedRdpBackend *q;
  std::unique_ptr<KRdp::Server> server;
  std::map<QString, std::unique_ptr<VirtualOutput>> outputs;
  std::map<QString, std::unique_ptr<Connection>> connections;
  ServerSettings settings;
  MonitorLayout layout;
  QString controllerId;
  bool listening = false;
  QProcess audioCapture;
  QByteArray audioRemainder;
  QString clientClipboardEcho;
};

EmbeddedRdpBackend::EmbeddedRdpBackend(QObject *parent)
    : RdpBackend(parent), d(std::make_unique<Private>(this)) {}

EmbeddedRdpBackend::~EmbeddedRdpBackend() { stop(); }

QString EmbeddedRdpBackend::name() const {
  return QStringLiteral("krdp-embedded");
}

bool EmbeddedRdpBackend::available() const {
  return qobject_cast<QGuiApplication *>(QCoreApplication::instance()) &&
         !qEnvironmentVariableIsEmpty("WAYLAND_DISPLAY");
}

bool EmbeddedRdpBackend::running() const { return d->listening; }

bool EmbeddedRdpBackend::start(const ServerSettings &settings,
                               const MonitorLayout &layout,
                               const QList<RdpCredential> &credentials,
                               QString *error) {
  if (!available()) {
    if (error) {
      *error = QStringLiteral("Embedded KRdp requires a QGuiApplication "
                              "connected to the Plasma Wayland session");
    }
    return false;
  }
  if (credentials.isEmpty()) {
    if (error) {
      *error = QStringLiteral("At least one RDP user is required");
    }
    return false;
  }

  stop();
  d->settings = settings;
  d->layout = layout;
  d->server = std::make_unique<KRdp::Server>();
  d->server->setAddress(QHostAddress(settings.address));
  d->server->setPort(settings.port);
  d->server->setTlsCertificate(settings.certificate.toStdString());
  d->server->setTlsCertificateKey(settings.certificateKey.toStdString());
  QList<KRdp::User> users;
  users.reserve(credentials.size());
  for (const auto &credential : credentials) {
    users.append({
        .name = credential.username,
        .password = credential.password,
        .readOnly = credential.readOnly,
    });
  }
  d->server->setUsers(users);
  QObject::connect(d->server.get(), &KRdp::Server::newConnectionCreated, this,
                   [this](KRdp::RdpConnection *connection) {
                     d->addConnection(connection);
                   });

  if (!d->ensureVirtualOutputs(layout, error)) {
    stop();
    return false;
  }
  if (!d->server->start()) {
    if (error) {
      *error = QStringLiteral("KRdp could not listen on %1:%2")
                   .arg(settings.address)
                   .arg(settings.port);
    }
    stop();
    return false;
  }
  if (!d->startAudio(error)) {
    stop();
    return false;
  }
  d->listening = true;
  Q_EMIT runningChanged();
  return true;
}

void EmbeddedRdpBackend::stop() {
  const bool wasRunning = d->listening;
  d->listening = false;
  d->stopAudio();
  if (d->server) {
    d->server->stop();
  }
  d->connections.clear();
  d->outputs.clear();
  d->server.reset();
  if (wasRunning) {
    Q_EMIT runningChanged();
  }
}

bool EmbeddedRdpBackend::prepareLayout(const MonitorLayout &layout,
                                       QString *error) {
  return d->ensureVirtualOutputs(layout, error);
}

bool EmbeddedRdpBackend::updateLayout(const MonitorLayout &layout, QString *) {
  d->layout = layout;
  const auto krdpLayout = toKrdpLayout(layout);
  for (auto &[id, state] : d->connections) {
    if (!state->connection) {
      continue;
    }
    state->connection->setMonitorLayout(krdpLayout);
    d->createCapture(*state);
  }
  return true;
}

void EmbeddedRdpBackend::setController(const QString &connectionId) {
  d->controllerId = connectionId;
  for (auto &[id, state] : d->connections) {
    if (!state->connection) {
      continue;
    }
    const bool enabled = id == connectionId && !state->readOnly;
    state->connection->inputHandler()->setEnabled(enabled);
    if (enabled && d->settings.clipboard) {
      const auto *clipboard =
          KSystemClipboard::instance()->mimeData(QClipboard::Clipboard);
      if (clipboard && clipboard->hasText()) {
        state->connection->clipboard()->setServerText(clipboard->text());
      }
    }
    state->connection->clipboard()->setEnabled(enabled &&
                                               d->settings.clipboard);
  }
}

bool EmbeddedRdpBackend::acceptClientLayout(const QString &connectionId,
                                            const MonitorLayout &layout,
                                            QString *error) {
  const auto iterator = d->connections.find(connectionId);
  if (iterator == d->connections.end() || !iterator->second->connection) {
    if (error) {
      *error =
          QStringLiteral("The requesting RDP connection is no longer active");
    }
    return false;
  }
  const auto monitors = toKrdpLayout(layout);
  if (!iterator->second->connection->displayControl()
           ->acceptRequestedMonitorLayout(monitors, error)) {
    return false;
  }
  return updateLayout(layout, error);
}

void EmbeddedRdpBackend::rejectClientLayout(const QString &connectionId) {
  const auto iterator = d->connections.find(connectionId);
  if (iterator != d->connections.end() && iterator->second->connection) {
    iterator->second->connection->displayControl()
        ->rejectRequestedMonitorLayout();
  }
}
