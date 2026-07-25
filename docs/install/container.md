# Docker and Podman

The OCI image is host-distribution neutral. Plasma and applications stay on
the host; only `kheadlessd` runs in the container.

Export the session values from a terminal inside the target Plasma session:

```sh
export KHEADLESS_UID="$(id -u)"
export KHEADLESS_GID="$(id -g)"
export KHEADLESS_RUNTIME_DIR="${XDG_RUNTIME_DIR}"
export KHEADLESS_WAYLAND="${WAYLAND_DISPLAY}"
export KHEADLESS_PIPEWIRE="${PIPEWIRE_REMOTE:-pipewire-0}"
export KHEADLESS_STATE_DIR="${XDG_STATE_HOME:-$HOME/.local/state}/kheadless"
mkdir -p "${KHEADLESS_STATE_DIR}"
docker compose -f container/compose.yaml up --build -d
```

For Podman, use the same Compose file through `podman compose`, or install the
Quadlet template from `container/kheadlessd.container` into
`~/.config/containers/systemd/`.

The container:

- runs as the desktop user;
- drops every Linux capability;
- uses `no-new-privileges` and a read-only root filesystem;
- mounts only Wayland, PipeWire, session D-Bus, and persistent KHeadless state;
- never mounts host `/`, the home directory, or the host PID namespace.

Rootless runtimes are preferred. See [security](../security.md) before opening
TCP 3389.
