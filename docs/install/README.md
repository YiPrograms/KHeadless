# Installation guides

KHeadless does not invoke a package manager. Choose a guide, install the
dependencies yourself, then run `tools/check-dependencies`.

- [Arch Linux, CachyOS, and Manjaro](arch.md)
- [Fedora](fedora.md)
- [Debian](debian.md)
- [Ubuntu and Kubuntu](ubuntu.md)
- [openSUSE](opensuse.md)
- [Generic systemd distribution](generic.md)
- [Docker and Podman](container.md)

The package-name source of truth is
[`docs/dependencies.yaml`](../dependencies.yaml). A supported host must run a
Plasma Wayland 6.3+ session. The container does not replace the host Plasma
session.

After installation, complete the [live interoperability checks](../testing.md)
before exposing the service outside a trusted test network.
