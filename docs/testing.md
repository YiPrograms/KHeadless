# Live interoperability testing

Run these checks from the target user's active Plasma Wayland session after
the automated test suite and staged installation have passed.

## Host preparation

```sh
systemctl --user import-environment \
  WAYLAND_DISPLAY DISPLAY XDG_CURRENT_DESKTOP XDG_SESSION_TYPE
systemctl --user daemon-reload
kbuildsycoca6 --noincremental
systemctl --user enable --now app-org.kde.kheadlessd.service
kheadlessctl passwd test-operator
kheadlessctl start
tools/live-smoke-test
journalctl --user -u app-org.kde.kheadlessd.service -f
```

Do not hardcode `wayland-0` in the installed unit. Plasma must populate the
user-manager environment. The smoke test can discover the active socket for
diagnostics, but the daemon itself requires `WAYLAND_DISPLAY`.

## FreeRDP

Use the physical host display for the local client; KHeadless captures only
its virtual outputs, avoiding a recursive desktop mirror.

```sh
xfreerdp3 /v:127.0.0.1:3389 /u:test-operator \
  /dynamic-resolution /sound /clipboard /cert:tofu

xfreerdp3 /v:127.0.0.1:3389 /u:test-operator \
  /multimon /dynamic-resolution /sound /clipboard /cert:tofu

xfreerdp3 /monitor-list
```

Verify window resizing, all selected monitors, pointer and keyboard mapping,
host playback audio, Unicode clipboard in both directions, reconnect, and
resolution changes while applications are running.

## Windows

From the existing Windows test machine, run these separately:

```text
mstsc.exe /v:HOST:3389 /dynamic-resolution
mstsc.exe /v:HOST:3389 /multimon
```

Test horizontal, vertical, negative-coordinate, mixed-resolution, mixed-DPI,
rotation, primary-display, add/remove, and reconnect topologies. Confirm the
layout in both `kheadlessctl monitors` and the settings UI.

## Concurrent-client policy

Connect a writable FreeRDP client and Windows client simultaneously, then add
a read-only account. The newest writable connection must be controller.
Viewers must receive graphics and audio but cannot inject input or exchange
clipboard data. Disconnecting the controller must promote the next-newest
writable client.

## Completion record

Record client versions, topology, graphics capability, pass/fail result, and
the matching journal interval. Before release, repeat topology changes and
reconnects for at least two hours and confirm that KWin output, PipeWire stream,
and RDPGFX surface counts return to the configured monitor count.
