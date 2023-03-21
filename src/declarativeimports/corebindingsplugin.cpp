/*
    SPDX-FileCopyrightText: 2009 Alan Alpert <alan.alpert@nokia.com>
    SPDX-FileCopyrightText: 2010 Ménard Alexis <menard@kde.org>
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "corebindingsplugin.h"

#include <QQmlContext>

#include <KLocalizedContext>

#include <plasmasvg/framesvg.h>
#include <plasmasvg/svg.h>

#include "framesvgitem.h"
#include "quicktheme.h"
#include "svgitem.h"
#include "theme.h"

#include <QDebug>
#include <QWindow>

void CoreBindingsPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);

    QQmlContext *context = engine->rootContext();

    Plasma::QuickTheme *theme = new Plasma::QuickTheme(engine);

    if (!qEnvironmentVariableIntValue("PLASMA_NO_CONTEXTPROPERTIES")) {
        context->setContextProperty(QStringLiteral("theme"), theme);
        context->setContextProperty(QStringLiteral("units"), &Units::instance());
    }

    if (!context->contextObject()) {
        KLocalizedContext *localizedContextObject = new KLocalizedContext(engine);
        context->setContextObject(localizedContextObject);
    }
}

void CoreBindingsPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.plasma.core"));

    qmlRegisterSingletonType<Units>(uri, 2, 0, "Units", [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        engine->setObjectOwnership(&Units::instance(), QQmlEngine::CppOwnership);
        return &Units::instance();
    });

    qmlRegisterType<Plasma::Svg>(uri, 2, 0, "Svg");
    qmlRegisterType<Plasma::FrameSvg>(uri, 2, 0, "FrameSvg");
    qmlRegisterType<Plasma::SvgItem>(uri, 2, 0, "SvgItem");
    qmlRegisterType<Plasma::FrameSvgItem>(uri, 2, 0, "FrameSvgItem");

    // qmlRegisterType<ThemeProxy>(uri, 2, 0, "Theme");
    qmlRegisterSingletonType<Plasma::QuickTheme>(uri, 2, 0, "Theme", [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        return new Plasma::QuickTheme(engine);
    });
}
