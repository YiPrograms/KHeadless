# Fedora

Fedora releases whose installed Plasma version is 6.3 or newer are supported.

```sh
plasmashell --version
sudo dnf install \
  gcc-c++ cmake ninja-build git pkgconf-pkg-config extra-cmake-modules \
  qt6-qtbase-devel qt6-qtbase-private-devel qt6-qtdeclarative-devel qt6-qtwayland-devel \
  kf6-kconfig-devel kf6-kdbusaddons-devel kf6-kcoreaddons-devel \
  kf6-kguiaddons-devel kf6-kcmutils-devel kf6-ki18n-devel \
  kf6-kstatusnotifieritem-devel kf6-kcrash-devel libkscreen-devel \
  kpipewire-devel plasma-wayland-protocols-devel \
  freerdp-devel libwinpr-devel qtkeychain-qt6-devel \
  wayland-devel libxkbcommon-devel libsodium-devel pipewire-devel pipewire-utils openssl
```

Fedora publishes both
[`libkscreen-devel`](https://packages.fedoraproject.org/pkgs/libkscreen/libkscreen-devel/)
and [`krdp-devel`](https://packages.fedoraproject.org/pkgs/krdp/krdp-devel/).
KHeadless builds its patched KRdp submodule and therefore does not link the
distribution `krdp-devel` package.

Build:

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

For a native install, label installed files through an RPM package in
production rather than permanently disabling SELinux. For containers, keep
SELinux enabled and add `:Z` only to the persistent state bind mount; user
runtime sockets should use `:z` if the runtime requires relabeling.

Open the RDP port only on the intended zone:

```sh
sudo firewall-cmd --permanent --zone=home --add-port=3389/tcp
sudo firewall-cmd --reload
```
