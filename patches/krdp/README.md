# KRdp downstream boundary

The downstream work is developed directly in the submodule on
`kheadless/multimonitor-v1`, based on KDE KRdp `v6.3.6` (`284f9df`). The branch
is local for now and can be pushed unchanged to the future fork remote.

## Current local commit stack

1. `4040145` — port the Plasma/6.3 base to FreeRDP 3.
2. `b72c03f` — restore builds with Qt 6.10 and newer.
3. `1a644ea` — implement and validate 1–16 monitor MS-RDPEDISP layouts.
4. `7260250` — publish the complete monitor topology through RDPGFX
   `ResetGraphics`; the current implementation maps one combined-workspace
   surface across the graphics output buffer.
5. `b8e5c16` — expose the compositor acceptance handshake and initial client
   monitor layout.
6. `34647b4` — expose authenticated username, peer address, and read-only
   metadata for KHeadless connection ownership.
7. `91c85f3` — install public headers and export `KRdp::KRdp`.
8. `154d10b` — gate input and clipboard access per connection.
9. `0e7a2a0` — publish one encoded monitor stream through each mapped RDPGFX
   surface while retaining combined-workspace compatibility.
10. `99c2c20` — negotiate and send 48 kHz stereo playback over `rdpsnd`.
11. `041d1a9` — transfer Unicode text through `cliprdr`, enforce the
    per-connection ownership gate, and advertise playback audio.
12. `3b080dd` — exercise all supported monitor counts and build the fork in
    GitHub Actions.
13. `867bc5a` — isolate and test Unicode and local-text clipboard codecs.
14. `c8503bb` — resolve the core Wayland protocol data directory explicitly
    for clean and containerized builds.

## Remaining upstream-hardening work

1. **NLA credential API:** FreeRDP completes NTLM authentication before KRdp
   receives a usable identity, so the current integration must generate a
   mode-0600, per-handshake SAM file from keyring secrets. Replace this only
   when FreeRDP exposes a server-side verifier suitable for NTLM challenge
   validation; an ordinary plaintext callback after NLA would not be secure or
   functional.
2. **Stable surface reuse:** output streams remain stable across arrangement
   changes, while an RDPGFX reset allocates new surface IDs. Reusing surfaces
   across compatible resets is an optimization rather than a correctness
   requirement.

## Public callback contract

The branch now exports `DisplayMonitor`, `DisplayMonitorList`, the client
layout request signal, the explicit accept/reject methods, and authenticated
connection metadata. KHeadless converts those values to its policy model
without exposing FreeRDP structures through D-Bus.

```cpp
struct DisplayMonitor {
    QPoint position;
    QSize size;
    QSize physicalSize;
    uint32_t orientation;
    uint32_t desktopScaleFactor;
    uint32_t deviceScaleFactor;
    bool primary;
};

signals:
    void requestedMonitorLayoutChanged(DisplayMonitorList);
    void authenticated(QString username, QString peer, bool readOnly);
```

KHeadless remains responsible for KWin virtual-output and KPipeWire session
lifecycle, layout policy, LibKScreen application, profiles, ownership election,
credentials, PipeWire playback capture, persistence, and D-Bus. KRdp remains
responsible for RDP wire protocols, transport security, RDPGFX surfaces,
input/clipboard gates, and audio channels.

## Compatibility tests required before enabling

- Windows `mstsc /multimon` and `/dynamic-resolution`.
- FreeRDP `/multimon`, `/monitors`, and `/dynamic-resolution`.
- Supported FreeRDP 3.x branches.
- One through sixteen displays, mixed rotations/scales, negative source
  coordinates, add/remove, resize, reconnect, and controller handoff.
- H.264 and progressive RDPGFX capability combinations.
