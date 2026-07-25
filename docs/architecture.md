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

The patched KRdp target is linked statically into `kheadlessd`. KHeadless
never installs `libKRdp.so`, so a fork based on one Plasma release cannot
replace or break the distribution's stock KRdp ABI.

Virtual monitors require a KWin output backend that implements virtual-output
creation. Normal host Plasma sessions using DRM, and KWin's virtual backend,
support this path. KWin's nested Wayland backend currently returns an
unregistered output for this protocol request; run KHeadless against the real
host session rather than inside a second nested Plasma session.

FreeRDP performs NLA/NTLM before exposing a connection to KRdp. KHeadless
therefore loads dedicated protocol secrets from the desktop keyring and KRdp
creates a mode-0600, per-handshake SAM file that is removed immediately after
authentication. The Argon2id store remains the management verifier; neither
secret is placed in settings, process arguments, or service environment.
