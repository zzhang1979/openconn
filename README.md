# OpenConn

Native Qt Quick frontend for the OpenConnect SSL VPN client, packaged as an
[Omarchy](https://omarchy.org/) plugin. It adds a live VPN status widget to the
Omarchy bar and a full-featured GUI for managing connections.

## Omarchy plugin (bar widget)

The plugin ships as `bar-widget` (`zzhang.openconn`) and places a shield
indicator in the right section of the bar:

- `🛡 VPN: <ip>` in emerald green when a tunnel is up, showing the IPv4 address
  assigned to `tun0`.
- `🛡 VPN: Off` in dim slate when there is no tunnel.
- Clicking the widget launches (or focuses) the OpenConn application.

Status is read by polling `ip -j -4 addr show tun0` every 2.5 seconds. The
widget makes no network requests of its own and writes no Omarchy or Hyprland
configuration.

### Requirements

| Dependency | Source |
|---|---|
| Omarchy with Quickshell support | Omarchy |
| Qt >= 6.12 | Omarchy |
| `openconnect` (`openconnect` package) | Arch extra repo |
| Qt 6 build tools: `qt6-base`, `qt6-declarative` | Arch extra repo |

Install the Arch packages listed above through your normal package manager if
they are missing. The table is the complete set; the plugin itself installs
nothing.

### Install

```bash
omarchy plugin add https://github.com/zzhang1979/openconn.git --enable
```

The plugin adds the bar widget only. To use the click target and the full GUI,
build and install the application into your user profile:

```bash
./bin/build
./bin/install
```

`bin/install` copies `openconn` into `~/.local/bin` and registers a desktop
entry and icon under `~/.local/share`. It runs entirely as your user and needs
no elevated permissions.

### Remove

```bash
omarchy plugin remove zzhang.openconn
```

To also remove the desktop application installed by `bin/install`:

```bash
rm -f ~/.local/bin/openconn \
      ~/.local/share/applications/openconn.desktop \
      ~/.local/share/icons/hicolor/scalable/apps/openconn.svg
```

## Application highlights

- Multi-profile management with local credential persistence
- Two-phase connection flow (unprivileged auth, then elevated tunnel setup)
- Interactive console input for 2FA/OTP prompts
- Automatic SSL server fingerprint retrieval
- Wayland and X11 support

## Credentials and privileges

Profiles store server addresses, protocols (Cisco AnyConnect, GlobalProtect,
Pulse, Fortinet, and others), usernames, passwords, and 2FA settings locally
with `0600` permissions. Tunnel creation and route cleanup go through polkit, so
no passwordless sudoers entries are installed.

## Build

```bash
./bin/build   # qmake6 + make, output in build/openconn
./bin/install # user-space install
./bin/test    # run the test suite
```

## License

[MIT](LICENSE)
