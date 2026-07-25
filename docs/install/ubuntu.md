# Ubuntu and Kubuntu

Only releases whose repositories provide Plasma 6.3+, Qt 6.7+, and KF6 6.10+
are supported. Check first:

```sh
plasmashell --version
qmake6 -query QT_VERSION
apt-cache policy libkscreen-dev libkf6kcmutils-dev
```

On a qualifying release:

```sh
sudo apt update
sudo apt install \
  build-essential cmake ninja-build git pkgconf extra-cmake-modules \
  qt6-base-dev qt6-base-private-dev qt6-declarative-dev qt6-wayland-dev \
  libkf6config-dev libkf6dbusaddons-dev libkf6coreaddons-dev \
  libkf6guiaddons-dev libkf6kcmutils-dev libkf6i18n-dev \
  libkf6statusnotifieritem-dev libkf6crash-dev libkscreen-dev libkpipewire-dev \
  plasma-wayland-protocols freerdp3-dev libwinpr3-dev \
  qtkeychain-qt6-dev libwayland-dev libxkbcommon-dev libsodium-dev \
  libpipewire-0.3-dev pipewire pipewire-bin openssl
```

Ubuntu package naming follows Debian. For example, Ubuntu publishes
[`qt6-wayland-dev`](https://packages.ubuntu.com/questing/qt6-wayland-dev).
If `apt` cannot resolve one of the KF6/Plasma 6 development packages, that
release is not supported for a native build; use a newer Kubuntu release or
the OCI path.

Build with the commands in the [Debian guide](debian.md), then follow
[session setup](../session.md).
