# openSUSE

Tumbleweed with Plasma 6.3 or newer is the maintained target. Leap is
supported only when all version checks in the generic guide pass.

```sh
sudo zypper refresh
sudo zypper install \
  gcc-c++ cmake ninja git pkgconf kf6-extra-cmake-modules \
  qt6-base-devel qt6-declarative-devel qt6-wayland-devel \
  kf6-kcoreaddons-devel kf6-kguiaddons-devel kf6-kcmutils-devel libkscreen6-devel \
  kpipewire6-devel plasma-wayland-protocols \
  freerdp-devel winpr-devel libqt6keychain-devel \
  libxkbcommon-devel libsodium-devel pipewire-devel pipewire pipewire-tools openssl
```

Package names occasionally transition during Tumbleweed snapshots. If a
listed package is renamed, locate the provider instead of omitting it:

```sh
zypper search -s kpipewire
zypper search -s libkscreen
zypper what-provides 'cmake(KF6Screen)'
```

Build:

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

Use the distribution firewall tooling to limit TCP 3389 to trusted networks.
Do not disable AppArmor globally; add a local profile adjustment only if its
audit log shows a denied KHeadless socket operation.
