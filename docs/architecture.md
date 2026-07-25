# Architecture

```text
Windows mstsc / FreeRDP
          │ RDP Display Control, RDPGFX, input, clipboard, rdpsnd
          ▼
      patched KRdp
          │ complete requested monitor layout
          ▼
      kheadlessd ───── org.kde.KHeadless1 ───── settings UI / KCM / CLI
          │
          ├── LibKScreen ── KWin virtual outputs and arrangement
          ├── KPipeWire ─── one video stream per monitor
          ├── PipeWire ──── default-sink playback capture
          └── policy ─────── profiles, rollback, ownership, credentials
```

Plasma runs natively on the host. The container boundary, when selected, is
between `kheadlessd` and the host session sockets—not between applications and
the host filesystem.

Monitor layout is the transaction boundary. KHeadless validates the complete
request, asks KWin to create/configure outputs, waits for all streams, resets
RDPGFX with all monitor definitions and surfaces, then publishes success.
Failure at any stage restores the last working layout.

Each configured monitor is owned by a long-lived Plasma screencast session.
Its encoded frames are fanned out to the per-connection KRdp transports, so
adding clients does not duplicate host-side video encoding. A lightweight
workspace session maps absolute input coordinates while the elected
connection is permitted to inject input.

FreeRDP performs NLA/NTLM before exposing a connection to KRdp. KHeadless
therefore loads dedicated protocol secrets from the desktop keyring and KRdp
creates a mode-0600, per-handshake SAM file that is removed immediately after
authentication. The Argon2id store remains the management verifier; neither
secret is placed in settings, process arguments, or service environment.
