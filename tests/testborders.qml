/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/


import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as QQC2

import org.kde.ksvg as KSvg

// This import is required to initialize Plasma Theme
import org.kde.plasma.core as PlasmaCore

Item {
    width: 500
    height: 500

    component EnabledBorderCheckBox : QQC2.CheckBox {
        required property int /*KSvg.FrameSvg.EnabledBorder*/ border

        anchors.centerIn: parent

        checked: (theItem.enabledBorders & border) !== KSvg.FrameSvg.NoBorder

        onToggled: {
            if (checked) {
                theItem.enabledBorders |= border;
            } else {
                theItem.enabledBorders &= ~border;
            }
        }
    }

    KSvg.FrameSvgItem {
        id: theItem

        imagePath: "widgets/background"
        anchors {
            fill: parent
            margins: 10
        }

        EnabledBorderCheckBox {
            text: "Left"
            border: KSvg.FrameSvg.LeftBorder
            anchors.horizontalCenterOffset: -50
        }

        EnabledBorderCheckBox {
            text: "Right"
            border: KSvg.FrameSvg.RightBorder
            anchors.horizontalCenterOffset: 50
        }

        EnabledBorderCheckBox {
            text: "Top"
            border: KSvg.FrameSvg.TopBorder
            anchors.verticalCenterOffset: -50
        }

        EnabledBorderCheckBox {
            text: "Bottom"
            border: KSvg.FrameSvg.BottomBorder
            anchors.verticalCenterOffset: 50
        }
    }
}
