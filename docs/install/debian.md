# Debian

Debian 13 (Trixie) or newer is supported. Debian 12’s default Plasma 5 stack
does not meet the Plasma 6.3 minimum. Trixie provides Plasma Workspace 6.3.6
and Qt 6.8 development files in
[`plasma-workspace-dev`](https://packages.debian.org/trixie/amd64/plasma-workspace-dev).

```sh
sudo apt update
sudo apt install \
  build-essential cmake ninja-build git pkgconf extra-cmake-modules \
  qt6-base-dev qt6-declarative-dev qt6-wayland-dev \
  libkf6coreaddons-dev libkf6guiaddons-dev libkf6kcmutils-dev libkscreen-dev libkpipewire-dev \
  plasma-wayland-protocols freerdp3-dev libwinpr3-dev \
  qtkeychain-qt6-dev libxkbcommon-dev libsodium-dev \
  libpipewire-0.3-dev pipewire pipewire-bin openssl
```

The FreeRDP package names are confirmed by Debian’s
[`freerdp3-dev`](https://packages.debian.org/trixie/amd64/devel/freerdp3-dev)
and [`libwinpr3-dev`](https://packages.debian.org/stable/libdevel/libwinpr3-dev)
pages.

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

Use `ufw allow from YOUR_LAN_CIDR to any port 3389 proto tcp` if UFW manages
the firewall. Replace `YOUR_LAN_CIDR`; do not expose the default listener to
the public internet.
