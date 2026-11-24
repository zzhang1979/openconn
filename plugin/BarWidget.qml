import QtQuick
import QtQuick.Layouts
import Quickshell
import Quickshell.Io
import qs.Commons
import qs.Ui

BarWidget {
  id: root
  moduleName: "zzhang.openconn"

  property bool isConnected: false
  property string assignedIp: ""

  readonly property color connectedColor: "#10B981" // Emerald green
  readonly property color disconnectedColor: "#64748B" // Slate dim

  implicitWidth: contentRow.implicitWidth + Style.space(8)
  implicitHeight: root.barSize

  // Periodic poll of tun0 status
  Timer {
    interval: 2500
    running: true
    repeat: true
    triggeredOnStart: true
    onTriggered: {
      if (!checkProcess.running) {
        checkProcess.running = true
      }
    }
  }

  Process {
    id: checkProcess
    running: false
    command: ["ip", "-j", "-4", "addr", "show", "tun0"]

    stdout: StdioCollector {
      waitForEnd: true
      onStreamFinished: {
        try {
          var trimmed = String(text || "").trim()
          if (trimmed.length > 0 && trimmed.startsWith("[")) {
            var data = JSON.parse(trimmed)
            if (data.length > 0 && data[0].addr_info && data[0].addr_info.length > 0) {
              root.assignedIp = data[0].addr_info[0].local || ""
              root.isConnected = true
              return
            }
          }
        } catch (e) {
          // Ignore JSON parse errors if device went down
        }
        root.isConnected = false
        root.assignedIp = ""
      }
    }
  }

  RowLayout {
    id: contentRow
    anchors.centerIn: parent
    spacing: Style.space(4)

    // VPN Shield Indicator
    Text {
      text: root.isConnected ? "🛡" : "🛡"
      font.pixelSize: Style.font.sizeDefault
      color: root.isConnected ? root.connectedColor : root.disconnectedColor
    }

    Text {
      text: root.isConnected ? (root.assignedIp ? ("VPN: " + root.assignedIp) : "VPN: Connected") : "VPN: Off"
      font.pixelSize: Style.font.sizeSmall
      font.family: root.bar ? root.bar.fontFamily : Style.font.family
      font.bold: root.isConnected
      color: root.isConnected ? "#F8FAFC" : root.disconnectedColor
    }
  }

  MouseArea {
    anchors.fill: parent
    cursorShape: Qt.PointingHandCursor
    hoverEnabled: true

    onClicked: {
      if (root.bar && typeof root.bar.run === "function") {
        root.bar.run("openconn")
      }
    }
  }
}
