/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

import QtQuick

import org.kde.ksvg as KSvg

// One element, stretched well beyond its own size, which is the case an item draws from a small texture.
KSvg.SvgItem {
    anchors.fill: parent
    imagePath: stretchImagePath
    elementId: stretchElementId
    smooth: false
}
