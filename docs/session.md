# Plasma session and automatic login

KHeadless controls an existing host Plasma Wayland session. The normal desktop
user therefore retains the same home, `/`, devices, keyring, portals, and
applications they have when sitting at the machine.

## Existing graphical session

Install and enable the user service:

```sh
systemctl --user daemon-reload
systemctl --user enable --now kheadlessd.service
kheadlessctl diagnostics
```

The diagnostic output must report a Wayland socket, PipeWire socket, and
connected session D-Bus.

## SDDM automatic login

Create `/etc/sddm.conf.d/kheadless.conf` locally:

```ini
[Autologin]
User=YOUR_DESKTOP_USER
Session=plasmawayland.desktop
Relogin=true
```

Replace `YOUR_DESKTOP_USER`. Session file names differ slightly across
distributions; inspect `/usr/share/wayland-sessions/` and use the Plasma
Wayland desktop filename installed there.

Automatic login grants anyone with physical access that desktop identity.
Use full-disk encryption and appropriate firmware/console controls when that
matters.

## Headless GPU considerations

KWin can create virtual outputs without a physical monitor. A functional DRM
render node is preferred. Verify `/dev/dri/renderD*`; on systems without one,
software rendering is possible but video encoding performance will be lower.
Do not grant the container broad `/dev` access—pass only the selected render
node.

