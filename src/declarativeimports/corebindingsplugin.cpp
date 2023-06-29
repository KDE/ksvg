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

#include <Kirigami/PlatformTheme>

#include "framesvgitem.h"
#include "imageset.h"
#include "svgitem.h"

#include <QDebug>
#include <QWindow>
#include <qquickitem.h>

void CoreBindingsPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
}

void CoreBindingsPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.ksvg"));

    qmlRegisterRevision<QQuickItem, 6>(uri, 1, 0);
    qmlRegisterAnonymousType<Kirigami::PlatformTheme>(uri, 1);
    qmlRegisterType<KSvg::Svg>(uri, 1, 0, "Svg");
    qmlRegisterType<KSvg::FrameSvg>(uri, 1, 0, "FrameSvg");
    qmlRegisterType<KSvg::SvgItem>(uri, 1, 0, "SvgItem");
    qmlRegisterType<KSvg::FrameSvgItem>(uri, 1, 0, "FrameSvgItem");
    qmlRegisterType<KSvg::ImageSet>(uri, 1, 0, "ImageSet");

    qmlRegisterSingletonType<KSvg::ImageSet>(uri, 1, 0, "ImageSet", [](QQmlEngine *, QJSEngine *) {
        return new KSvg::ImageSet;
    });
}

#include "moc_corebindingsplugin.cpp"
