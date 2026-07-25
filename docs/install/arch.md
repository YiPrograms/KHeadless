# Arch Linux, CachyOS, and Manjaro

Rolling releases with Plasma 6.3 or newer are supported. Update the system
before building; partial upgrades are not supported.

```sh
sudo pacman -Syu
sudo pacman -S --needed \
  base-devel cmake ninja git pkgconf extra-cmake-modules \
  qt6-base qt6-declarative qt6-wayland \
  kcoreaddons kguiaddons kcmutils libkscreen kpipewire plasma-wayland-protocols \
  freerdp qtkeychain-qt6 libxkbcommon libsodium pipewire pipewire-audio openssl
```

Arch’s current `krdp` package lists FreeRDP, KPipeWire, Qt 6, KF6, Wayland,
Extra CMake Modules, and Plasma Wayland protocols among its dependencies and
build dependencies; see the [official Arch package
metadata](https://archlinux.org/packages/extra/x86_64/krdp/).

Build:

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

Then follow [session setup](../session.md). Docker is available directly;
Podman users may install `podman` instead.
