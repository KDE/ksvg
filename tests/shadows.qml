/*
    SPDX-FileCopyrightText: 2021 Arjen Hiemstra <ahiemstra@heimr.nl>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Layouts

import org.kde.ksvg as KSvg

// This import is required to initialize Plasma Theme
import org.kde.plasma.core as PlasmaCore

Rectangle {
    color: "white"
    width: 600
    height: 600

    GridLayout {
        LayoutMirroring.enabled: false
        anchors.fill: parent

        columns: 3
        columnSpacing: 0
        rowSpacing: 0

        // 0 1 2
        // 3 4 5
        // 6 7 8
        flow: GridLayout.LeftToRight

        Repeater {
            model: [
                "shadow-topleft",
                "shadow-top",
                "shadow-topright",
                "shadow-left",
                "shadow-middle",
                "shadow-right",
                "shadow-bottomleft",
                "shadow-bottom",
                "shadow-bottomright"
            ]

            KSvg.SvgItem {
                required property int index
                required property string modelData

                Layout.fillWidth: [1, 4, 7].includes(index)
                Layout.fillHeight: [3, 4, 5].includes(index)

                elementId: modelData

                svg: KSvg.Svg {
                    imagePath: "dialogs/background"
                }
            }
        }
    }
}
