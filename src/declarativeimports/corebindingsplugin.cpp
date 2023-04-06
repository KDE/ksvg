/*
    SPDX-FileCopyrightText: 2009 Alan Alpert <alan.alpert@nokia.com>
    SPDX-FileCopyrightText: 2010 Ménard Alexis <menard@kde.org>
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "corebindingsplugin.h"

#include <QQmlContext>

#include <ksvg/framesvg.h>
#include <ksvg/svg.h>

#include "framesvgitem.h"
#include "svgitem.h"

#include <QDebug>
#include <QWindow>

void CoreBindingsPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);


}

void CoreBindingsPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.ksvg"));

    qmlRegisterType<KSvg::Svg>(uri, 2, 0, "Svg");
    qmlRegisterType<KSvg::FrameSvg>(uri, 2, 0, "FrameSvg");
    qmlRegisterType<KSvg::SvgItem>(uri, 2, 0, "SvgItem");
    qmlRegisterType<KSvg::FrameSvgItem>(uri, 2, 0, "FrameSvgItem");
}
