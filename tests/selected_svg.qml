/*
    SPDX-FileCopyrightText: 2016 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.ksvg as KSvg

// This import is required to initialize Plasma Theme
import org.kde.plasma.core as PlasmaCore

KSvg.FrameSvgItem {
    id: root

    width: 600
    height: 800

    imagePath: "widgets/background"
    status: KSvg.Svg.Normal

    component SvgStatusRadioButton : QQC2.RadioButton {
        required property /*KSvg.Svg.Normal*/int status
        Layout.fillWidth: true
        QQC2.ButtonGroup.group: group
        checked: root.status === status
        onToggled: {
            root.status = status;
        }
    }

    QQC2.ButtonGroup {
        id: group
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: 8

        QQC2.GroupBox {
            title: "Switch SVG status"

            ColumnLayout {
                anchors.fill: parent
                spacing: 4

                SvgStatusRadioButton {
                    text: "Normal"
                    status: KSvg.Svg.Normal
                }

                SvgStatusRadioButton {
                    text: "Selected"
                    status: KSvg.Svg.Selected
                }

                SvgStatusRadioButton {
                    text: "Inactive"
                    status: KSvg.Svg.Inactive
                }
            }
        }

        RowLayout {
            spacing: 4

            QQC2.Label {
                text: "Phone SVG:"
            }

            KSvg.SvgItem {
                svg: KSvg.Svg {
                    imagePath: "icons/phone"
                    status: root.status
                }
            }
        }
    }
}
