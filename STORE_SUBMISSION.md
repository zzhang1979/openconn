# Omarchy Marketplace Submission Draft

Copy and paste the details below into the Omarchy plugin marketplace issue template:
👉 **[Submit Plugin Issue](https://github.com/omacom/omarchy-plugin-marketplace/issues/new?template=submit-plugin.yml)**

---

### Plugin Information

- **Plugin Name**: OpenConn
- **Plugin ID**: `zzhang.openconn`
- **Repository URL**: `https://github.com/zzhang1979/openconn`
- **Category**: Network
- **Tags**: `Bar`, `Network`, `VPN`, `OpenConnect`, `Quickshell`
- **License**: MIT
- **Version**: `1.0.0`

### Description
> OpenConnect SSL VPN client and Omarchy bar widget with multi-profile credentials vault, 1-click passwordless connect/disconnect, live TUN interface telemetry, and terminal output.

### Features
- **Omarchy Bar Widget**: Live status indicator (🛡 Connected/Off) with real-time assigned VPN IP display directly in the Omarchy bar.
- **One-Click Native Client**: Built in C++17 & Qt 6 Quick / Material Dark, integrating directly with Omarchy desktop themes.
- **Credential & Multi-Profile Vault**: Encrypted-at-rest (`0600`) profile storage for server addresses, protocols (Cisco AnyConnect, GlobalProtect, Pulse, Fortinet, etc.), usernames, passwords, and 2FA settings.
- **Passwordless Polkit Integration**: Zero repetitive root password prompts for tunnel creation and route cleanup.
- **Interactive Terminal & OTP Support**: Handles dynamic server prompts, self-signed certificate pins, and 2FA/MFA token responses.

### Runtime Requirements
- Omarchy >= 4.5
- Qt >= 6.12 & Quickshell
- `openconnect` binary (`sudo pacman -S openconnect`)

### Installation Command
```bash
omarchy plugin add https://github.com/zzhang1979/openconn.git --enable
```

### Preview Image
`preview.png` is located at the root of the repository.
