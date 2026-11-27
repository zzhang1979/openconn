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
    Shortcut { sequence: "Ctrl+N"; context: Qt.ApplicationShortcut; onActivated: backend.addProfile() }
    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: root.showHelp = !root.showHelp }

    Shortcut { sequence: "Ctrl+M"; context: Qt.ApplicationShortcut; onActivated: root.showMinimized() }
    Shortcut { sequence: "Ctrl+W"; context: Qt.ApplicationShortcut; onActivated: root.hide() }

    function protocolIndex(val) {
        for (var i = 0; i < protocols.length; i++) {
            if (protocols[i].value === val) return i;
        }
        return 0;
    }

    // Root layout
    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14
        spacing: 10

        // Top App Bar with Minimize and Tray controls
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Image {
                source: "qrc:/openconn.svg"
                sourceSize.width: 24
                sourceSize.height: 24
            }

            Label {
                text: "OpenConn"
                font.pixelSize: 16
                font.bold: true
                color: "#F8FAFC"
            }

            Rectangle {
                color: "#334155"
                radius: 4
                implicitWidth: verLabel.implicitWidth + 10
                implicitHeight: verLabel.implicitHeight + 4
                Label {
                    id: verLabel
                    anchors.centerIn: parent
                    text: "v" + backend.appVersion
                    font.pixelSize: 10
                    font.bold: true
                    color: "#94A3B8"
                }
            }

            Label {
                text: "SSL VPN Client"
                font.pixelSize: 12
                color: "#64748B"
            }

            Item { Layout.fillWidth: true }

            Button {
                text: "Shortcuts (?)"
                flat: true
                onClicked: root.showHelp = !root.showHelp
            }

            Button {
                text: "Policy Info"
                flat: true
                onClicked: root.showPolkitInfo = true
            }

            Button {
                text: "— Minimize"
                flat: true
                ToolTip.visible: hovered
                ToolTip.text: "Minimize Window (Ctrl+M)"
                onClicked: root.showMinimized()
            }

            Button {
                text: "🗖 Hide to Tray"
                flat: true
                Material.foreground: theme.accent
                ToolTip.visible: hovered
                ToolTip.text: "Hide to System Tray (Ctrl+W)"
                onClicked: root.hide()
            }
        }

        // OpenConnect Missing Prerequisite Banner
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 38
            color: "#7F1D1D"
            radius: theme.radius
            visible: !backend.isOpenconnectInstalled

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 12
                spacing: 8

                Label {
                    text: "⚠️ Warning: OpenConnect binary not found on your PATH. Please install 'openconnect' to establish VPN tunnels."
                    font.pixelSize: 12
                    font.bold: true
                    color: "#FECACA"
                    Layout.fillWidth: true
                }
            }
        }

        // Main panels layout
        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
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
                            text: backend.profileName
                            placeholderText: "e.g. Work VPN"
                            selectByMouse: true
                            onTextEdited: backend.profileName = text
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
                            currentIndex: root.protocolIndex(backend.profileProtocol)
                            onActivated: backend.profileProtocol = protocols[currentIndex].value
                        }

                        Label {
                            text: "Server / Gateway Address"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        TextField {
                            id: serverField
                            Layout.fillWidth: true
                            text: backend.profileServer
                            placeholderText: "vpn.example.com or https://vpn..."
                            selectByMouse: true
                            onTextEdited: backend.profileServer = text
                        }

                        Label {
                            text: "Username"
                            color: "#94A3B8"
                            font.pixelSize: 12
                        }
                        TextField {
                            id: usernameField
                            Layout.fillWidth: true
                            text: backend.profileUsername
                            placeholderText: "user@domain.com"
                            selectByMouse: true
                            onTextEdited: backend.profileUsername = text
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
                                text: backend.profilePassword
                                placeholderText: "Saved password"
                                echoMode: root.passwordVisible ? TextInput.Normal : TextInput.Password
                                selectByMouse: true
                                onTextEdited: backend.profilePassword = text
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
                                text: backend.profileAuthgroup
                                placeholderText: "e.g. Employee-VPN or RA-VPN"
                                selectByMouse: true
                                onTextEdited: backend.profileAuthgroup = text
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                Label {
                                    text: "Server Cert Fingerprint"
                                    color: "#94A3B8"
                                    font.pixelSize: 12
                                }
                                Item { Layout.fillWidth: true }
                                Button {
                                    text: "Fetch Fingerprint"
                                    flat: true
                                    Material.foreground: theme.accent
                                    onClicked: backend.fetchCertificateFingerprint()
                                }
                            }
                            TextField {
                                id: certField
                                Layout.fillWidth: true
                                text: backend.profileServercert
                                placeholderText: "pin-sha256:... or sha1:..."
                                selectByMouse: true
                                onTextEdited: backend.profileServercert = text
                            }

                            Switch {
                                id: selfSignedSwitch
                                text: "Allow untrusted/self-signed certs"
                                checked: backend.profileAllowSelfSigned
                                onToggled: backend.profileAllowSelfSigned = checked
                            }

                            Label {
                                text: "Additional Arguments (Optional)"
                                color: "#94A3B8"
                                font.pixelSize: 12
                            }
                            TextField {
                                id: extraArgsField
                                Layout.fillWidth: true
                                text: backend.profileExtraArgs
                                placeholderText: "--disable-ipv6 ..."
                                selectByMouse: true
                                onTextEdited: backend.profileExtraArgs = text
                            }
                        }

                        Item { height: 8 }

                        Label {
                            text: "✓ Settings save automatically as you type"
                            font.pixelSize: 11
                            color: "#10B981"
                            Layout.alignment: Qt.AlignHCenter
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
                                text: backend.connectionState === 2 ? ("Target: " + (backend.profileServer || "VPN")) : (backend.profileServer ? ("Configured: " + backend.profileServer) : "Ready to configure")
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

                    // Header row with Clear button
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

                    // Certificate Alert Banner when prompt is detected
                    Rectangle {
                        visible: backend.waitingForCertPrompt
                        Layout.fillWidth: true
                        height: 48
                        color: "#78350F"
                        radius: 6

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 10

                            Label {
                                text: "⚠️ Server certificate not trusted by system CA."
                                color: "#FEF3C7"
                                font.pixelSize: 12
                                Layout.fillWidth: true
                            }

                            Button {
                                text: "Trust & Accept (yes)"
                                highlighted: true
                                Material.background: "#10B981"
                                onClicked: backend.acceptServerCert()
                            }

                            Button {
                                text: "Abort (no)"
                                flat: true
                                Material.foreground: "#FCA5A5"
                                onClicked: backend.rejectServerCert()
                            }
                        }
                    }

                    // Console output area
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

                    // Interactive Terminal Input Row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8

                        TextField {
                            id: terminalInput
                            Layout.fillWidth: true
                            placeholderText: "Type response (e.g. yes, 2FA code, OTP) and press Enter..."
                            selectByMouse: true
                            onAccepted: {
                                if (text.trim().length > 0) {
                                    backend.sendInput(text.trim());
                                    text = "";
                                }
                            }
                        }

                        Button {
                            text: "Send"
                            highlighted: true
                            onClicked: {
                                if (terminalInput.text.trim().length > 0) {
                                    backend.sendInput(terminalInput.text.trim());
                                    terminalInput.text = "";
                                }
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
