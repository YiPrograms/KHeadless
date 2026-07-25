# Generic systemd distribution

## Minimum contract

- Plasma/KWin/LibKScreen/KPipeWire 6.3
- Qt 6.7
- KDE Frameworks 6.10
- FreeRDP/WinPR 3.1
- PipeWire 0.3
- CMake 3.24, Ninja, pkg-config, a C++20 compiler, and libsodium
- systemd user services and a Plasma Wayland session

Map the logical names in [`docs/dependencies.yaml`](../dependencies.yaml) to
your distribution’s packages. Confirm the SDK:

```sh
tools/check-dependencies
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=RelWithDebInfo \
  -DKHEADLESS_BUILD_UI=ON \
  -DKHEADLESS_BUILD_KCM=ON \
  -DKHEADLESS_BUILD_KRDP=ON
cmake --build build
ctest --test-dir build --output-on-failure
sudo cmake --install build
```

The KCM is automatically disabled when Extra CMake Modules or KF6 KCM
development files are absent. The standalone settings application remains
available.

Native mode links the host Qt/KF6/LibKScreen. OCI mode ships the daemon’s user
space but still requires the host’s KWin virtual-output protocol, PipeWire,
and session D-Bus.
