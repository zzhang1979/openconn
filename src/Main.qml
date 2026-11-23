import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts

ApplicationWindow {
    id: root
    width: 980
    height: 720
    minimumWidth: 800
    minimumHeight: 600
    visible: true
    title: "OpenConn — VPN Client"
    color: "#0F172A"

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool showHelp: false
    property bool showPolkitInfo: false
    property bool passwordVisible: false
    property var protocols: [
        { text: "Cisco AnyConnect / ocserv", value: "anyconnect" },
        { text: "Palo Alto GlobalProtect", value: "gp" },
        { text: "Pulse Connect Secure", value: "pulse" },
        { text: "Juniper Network Connect", value: "nc" },
        { text: "Fortinet FortiGate", value: "fortinet" },
        { text: "F5 BIG-IP", value: "f5" },
        { text: "Array Networks", value: "array" }
    ]

    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: backend.toggleConnect() }
    Shortcut { sequence: "Ctrl+S"; context: Qt.ApplicationShortcut; onActivated: saveActiveProfile() }
    Shortcut { sequence: "Ctrl+N"; context: Qt.ApplicationShortcut; onActivated: backend.addProfile() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: root.showHelp = !root.showHelp }

    function protocolIndex(val) {
        for (var i = 0; i < protocols.length; i++) {
            if (protocols[i].value === val) return i;
        }
        return 0;
    }

    function saveActiveProfile() {
        var p = backend.activeProfile;
        p.name = nameField.text.trim();
        p.server = serverField.text.trim();
        p.protocol = protocols[protocolCombo.currentIndex].value;
        p.username = usernameField.text.trim();
        p.password = passwordField.text;
        p.authgroup = authgroupField.text.trim();
        p.servercert = certField.text.trim();
        p.allowSelfSigned = selfSignedSwitch.checked;
        p.extraArgs = extraArgsField.text.trim();
        backend.updateProfile(backend.activeIndex, p);
    }

    // Main layout
    RowLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 16

        // Left Panel: Profiles and Configuration
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 440
            Layout.minimumWidth: 380
            color: "#1E293B"
            radius: theme.radius

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                // Header & Profile Selector
                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "Profiles"
                        font.pixelSize: 16
                        font.bold: true
                        color: "#F8FAFC"
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        text: "+ New"
                        flat: true
                        Material.foreground: theme.accent
                        onClicked: backend.addProfile()
                    }

                    Button {
                        text: "Delete"
                        flat: true
                        enabled: backend.profiles.length > 1
                        Material.foreground: "#EF4444"
                        onClicked: backend.removeProfile(backend.activeIndex)
                    }
                }

                ComboBox {
                    id: profileSelector
                    Layout.fillWidth: true
                    model: backend.profiles
                    textRole: "name"
                    currentIndex: backend.activeIndex
                    onActivated: function(index) {
                        saveActiveProfile();
                        backend.activeIndex = index;
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: 1
                    color: "#334155"
                }

                // Form Scroll Area
                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    contentWidth: formLayout.width

                    ColumnLayout {
                        id: formLayout
                        width: 400
                        spacing: 10

                        Label {
                            text: "Profile Name"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        TextField {
                            id: nameField
                            Layout.fillWidth: true
                            text: backend.activeProfile.name || ""
                            placeholderText: "e.g. Work VPN"
                            selectByMouse: true
                            onEditingFinished: saveActiveProfile()
                        }

                        Label {
                            text: "Protocol"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        ComboBox {
                            id: protocolCombo
                            Layout.fillWidth: true
                            model: protocols
                            textRole: "text"
                            currentIndex: root.protocolIndex(backend.activeProfile.protocol)
                            onActivated: saveActiveProfile()
                        }

                        Label {
                            text: "Server / Gateway Address"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        TextField {
                            id: serverField
                            Layout.fillWidth: true
                            text: backend.activeProfile.server || ""
                            placeholderText: "vpn.example.com or https://vpn..."
                            selectByMouse: true
                            onEditingFinished: saveActiveProfile()
                        }

                        Label {
                            text: "Username"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        TextField {
                            id: usernameField
                            Layout.fillWidth: true
                            text: backend.activeProfile.username || ""
                            placeholderText: "user@domain.com"
                            selectByMouse: true
                            onEditingFinished: saveActiveProfile()
                        }

                        Label {
                            text: "Password"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        RowLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            TextField {
                                id: passwordField
                                Layout.fillWidth: true
                                text: backend.activeProfile.password || ""
                                placeholderText: "Saved password"
                                echoMode: root.passwordVisible ? TextInput.Normal : TextInput.Password
                                selectByMouse: true
                                onEditingFinished: saveActiveProfile()
                            }

                            Button {
                                text: root.passwordVisible ? "Hide" : "Show"
                                flat: true
                                onClicked: root.passwordVisible = !root.passwordVisible
                            }
                        }

                        // Advanced options accordion
                        RowLayout {
                            Layout.fillWidth: true
                            Button {
                                text: advancedDetails.visible ? "▲ Hide Advanced Options" : "▼ Show Advanced Options"
                                flat: true
                                onClicked: advancedDetails.visible = !advancedDetails.visible
                            }
                        }

                        ColumnLayout {
                            id: advancedDetails
                            visible: false
                            Layout.fillWidth: true
                            spacing: 8

                            Label {
                                text: "Authgroup / Realm (Optional)"
                                color: "#94A3B8"
                                font.pixelSize: 12
                            }
                            TextField {
                                id: authgroupField
                                Layout.fillWidth: true
                                text: backend.activeProfile.authgroup || ""
                                placeholderText: "e.g. Employee-VPN"
                                selectByMouse: true
                                onEditingFinished: saveActiveProfile()
                            }

                            Label {
                                text: "Server Cert Fingerprint (Optional)"
                                color: "#94A3B8"
                                font.pixelSize: 12
                            }
                            TextField {
                                id: certField
                                Layout.fillWidth: true
                                text: backend.activeProfile.servercert || ""
                                placeholderText: "pin-sha256:... or sha1:..."
                                selectByMouse: true
                                onEditingFinished: saveActiveProfile()
                            }

                            Switch {
                                id: selfSignedSwitch
                                text: "Allow untrusted/self-signed certs"
                                checked: backend.activeProfile.allowSelfSigned || false
                                onToggled: saveActiveProfile()
                            }

                            Label {
                                text: "Additional Arguments (Optional)"
                                color: "#94A3B8"
                                font.pixelSize: 12
                            }
                            TextField {
                                id: extraArgsField
                                Layout.fillWidth: true
                                text: backend.activeProfile.extraArgs || ""
                                placeholderText: "--disable-ipv6 ..."
                                selectByMouse: true
                                onEditingFinished: saveActiveProfile()
                            }
                        }

                        Item { height: 8 }

                        Button {
                            Layout.fillWidth: true
                            text: "Save Configuration (Ctrl+S)"
                            highlighted: true
                            onClicked: saveActiveProfile()
                        }
                    }
                }
            }
        }

        // Right Panel: Dashboard and Live Logs
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            // VPN Status & Action Card
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 220
                color: "#1E293B"
                radius: theme.radius

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 18
                    spacing: 14

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        // State Dot
                        Rectangle {
                            width: 14
                            height: 14
                            radius: 7
                            color: backend.connectionState === 2 ? "#10B981" :
                                   backend.connectionState === 1 ? "#F59E0B" :
                                   backend.connectionState === 4 ? "#EF4444" : "#64748B"
                        }

                        ColumnLayout {
                            spacing: 2
                            Label {
                                text: backend.statusText
                                font.pixelSize: 18
                                font.bold: true
                                color: "#F8FAFC"
                            }
                            Label {
                                text: backend.connectionState === 2 ? ("Target: " + (backend.activeProfile.server || "VPN")) : "Ready to connect"
                                font.pixelSize: 12
                                color: "#94A3B8"
                            }
                        }

                        Item { Layout.fillWidth: true }

                        Button {
                            text: "Policy Info"
                            flat: true
                            onClicked: root.showPolkitInfo = true
                        }
                    }

                    // Quick Stats Grid
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 12

                        Rectangle {
                            Layout.fillWidth: true
                            height: 52
                            color: "#0F172A"
                            radius: 6

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                Label { text: "Assigned IP"; font.pixelSize: 11; color: "#94A3B8" }
                                Label { text: backend.assignedIp; font.pixelSize: 13; font.bold: true; color: "#F8FAFC" }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 52
                            color: "#0F172A"
                            radius: 6

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                Label { text: "Interface"; font.pixelSize: 11; color: "#94A3B8" }
                                Label { text: backend.vpnInterface; font.pixelSize: 13; font.bold: true; color: "#F8FAFC" }
                            }
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            height: 52
                            color: "#0F172A"
                            radius: 6

                            ColumnLayout {
                                anchors.centerIn: parent
                                spacing: 2
                                Label { text: "Connected Time"; font.pixelSize: 11; color: "#94A3B8" }
                                Label { text: backend.connectedTime; font.pixelSize: 13; font.bold: true; color: "#F8FAFC" }
                            }
                        }
                    }

                    // Connect / Disconnect Big Button
                    Button {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 48
                        text: backend.connectionState === 2 ? "Disconnect VPN (Space)" :
                              backend.connectionState === 1 ? "Connecting... (Click to cancel)" :
                              "Connect to VPN (Space)"
                        highlighted: backend.connectionState !== 2
                        Material.background: backend.connectionState === 2 ? "#EF4444" : theme.accent
                        Material.foreground: backend.connectionState === 2 ? "#FFFFFF" : theme.accentForeground
                        font.pixelSize: 15
                        font.bold: true
                        onClicked: {
                            saveActiveProfile();
                            backend.toggleConnect();
                        }
                    }
                }
            }

            // Live Output Console Card
            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "#1E293B"
                radius: theme.radius

                ColumnLayout {
                    anchors.fill: parent
                    anchors.margins: 16
                    spacing: 8

                    RowLayout {
                        Layout.fillWidth: true
                        Label {
                            text: "Connection Output & Diagnostics"
                            font.bold: true
                            font.pixelSize: 14
                            color: "#F8FAFC"
                        }
                        Item { Layout.fillWidth: true }
                        Button {
                            text: "Clear"
                            flat: true
                            onClicked: backend.clearLogs()
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        color: "#0B0F19"
                        radius: 6

                        ScrollView {
                            id: logScroll
                            anchors.fill: parent
                            anchors.margins: 8
                            clip: true

                            TextArea {
                                id: logArea
                                text: backend.logs
                                readOnly: true
                                wrapMode: TextEdit.Wrap
                                font.family: "monospace"
                                font.pixelSize: 11
                                color: "#A7F3D0"
                                selectByMouse: true
                                background: null
                                onTextChanged: {
                                    logScroll.ScrollBar.vertical.position = 1.0 - logScroll.ScrollBar.vertical.size;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // Help Dialog
    Dialog {
        id: helpModal
        title: "Keyboard Shortcuts"
        visible: root.showHelp
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Ok
        onAccepted: root.showHelp = false
        onClosed: root.showHelp = false

        ColumnLayout {
            spacing: 8
            Label { text: "Space: Toggle Connect / Disconnect"; color: "#E2E8F0" }
            Label { text: "Ctrl+S: Save current profile"; color: "#E2E8F0" }
            Label { text: "Ctrl+N: Add new profile"; color: "#E2E8F0" }
            Label { text: "Q: Quit application"; color: "#E2E8F0" }
            Label { text: "?: Toggle this help dialog"; color: "#E2E8F0" }
        }
    }

    // Polkit Dialog
    Dialog {
        id: polkitModal
        title: "Passwordless VPN Setup (Optional)"
        visible: root.showPolkitInfo
        modal: true
        anchors.centerIn: parent
        standardButtons: Dialog.Close
        onAccepted: root.showPolkitInfo = false
        onClosed: root.showPolkitInfo = false

        ColumnLayout {
            spacing: 10
            width: 500

            Label {
                text: "By default, OpenConnect needs root privileges to create tun interfaces. Polkit will prompt you for your user password when connecting."
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: "#E2E8F0"
            }

            Label {
                text: "If you want 1-click connection without entering your password, add this rule to /etc/polkit-1/rules.d/50-openconnect.rules:"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
                color: "#94A3B8"
            }

            TextArea {
                Layout.fillWidth: true
                readOnly: true
                text: backend.getPolkitRuleSnippet()
                font.family: "monospace"
                font.pixelSize: 11
                color: "#F8FAFC"
                background: Rectangle { color: "#0F172A"; radius: 4 }
            }
        }
    }
}
