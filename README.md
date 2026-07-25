# KHeadless

KHeadless is a management layer for a host-native KDE Plasma Wayland session
exposed over RDP with KRdp. It provides:

- validated layouts for up to 16 virtual monitors;
- client-following, manual, and saved-profile display modes;
- transactional layout changes with a 15-second safety rollback;
- a session D-Bus API and `kheadlessctl`;
- a standalone Qt Quick settings application and Plasma KCM;
- dedicated Argon2id-hashed RDP users and automatic TLS certificates;
- connection ownership and diagnostics models;
- native systemd and Docker/Podman deployment assets;
- one KWin/KPipeWire stream and one RDPGFX surface per monitor;
- 48 kHz stereo host playback forwarding through RDP audio.

Plasma and applications run on the host. A container, when used, contains only
the coordinator and KRdp stack and receives narrowly scoped Wayland, PipeWire,
and session D-Bus socket mounts.

## Project status

The embedded backend is the default. It creates persistent KWin virtual
outputs, applies their arrangement through LibKScreen, streams every output
through a dedicated KPipeWire/RDPGFX surface, follows Display Control changes,
gates input and clipboard access to the elected controller, and forwards host
playback through `rdpsnd`. The stock `krdpserver` process backend remains
available in builds configured with `-DKHEADLESS_BUILD_KRDP=OFF`, but is
limited to one monitor.

## Build

Read the guide for your distribution under [`docs/install`](docs/install).
The common build is:

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DKHEADLESS_BUILD_KRDP=ON
cmake --build build
ctest --test-dir build --output-on-failure
cmake --install build
```

The automated suite is followed by the
[live interoperability matrix](docs/testing.md) for FreeRDP and Windows mstsc.

Run the daemon and inspect it:

```sh
systemctl --user daemon-reload
kbuildsycoca6 --noincremental
systemctl --user enable --now app-org.kde.kheadlessd.service
kheadlessctl status
kheadless-settings
```

Layouts accepted by `kheadlessctl apply` use this form:

```json
{
  "monitors": [
    {
      "id": "KHEADLESS-1",
      "name": "Primary",
      "x": 0,
      "y": 0,
      "width": 2560,
      "height": 1440,
      "scale": 1.0,
      "rotation": 0,
      "enabled": true,
      "primary": true
    }
  ]
}
```

## Security

Never place host or RDP passwords in command-line arguments, unit files, or
Compose environment variables. `kheadlessctl passwd USER` reads a password
from standard input, stores a libsodium Argon2id verifier in a mode-0600 file,
and places the KRdp protocol secret in the desktop keyring. TLS keys are also
mode 0600. The default listen address is
`0.0.0.0`; configure a host firewall before enabling the RDP backend.

## Supported platform

The supported host contract is systemd Linux, Plasma/KWin Wayland 6.3 or
newer, and a functioning PipeWire user session. Plasma 5 and non-systemd init
integration are outside the first release.
