/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "theme.h"
#include "private/svg_p.h"
#include "private/theme_p.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QMutableListIterator>
#include <QPair>
#include <QStringBuilder>
#include <QThread>
#include <QTimer>

#include <KColorScheme>
#include <KConfigGroup>
#include <KDirWatch>
#include <KImageCache>

#include "debug_p.h"

namespace KSvg
{
Theme::Theme(QObject *parent)
    : QObject(parent)
{
    if (!ThemePrivate::globalTheme) {
        ThemePrivate::globalTheme = new ThemePrivate;
        ThemePrivate::globalTheme->settingsChanged(false);
        if (QCoreApplication::instance()) {
            connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, ThemePrivate::globalTheme, &ThemePrivate::onAppExitCleanup);
        }
    }
    ThemePrivate::globalTheme->ref.ref();
    d = ThemePrivate::globalTheme;

    connect(d, &ThemePrivate::themeChanged, this, &Theme::themeChanged);
}

Theme::Theme(const QString &themeName, QObject *parent)
    : QObject(parent)
{
    auto &priv = ThemePrivate::themes[themeName];
    if (!priv) {
        priv = new ThemePrivate;
        if (QCoreApplication::instance()) {
            connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, priv, &ThemePrivate::onAppExitCleanup);
        }
    }

    priv->ref.ref();
    d = priv;

    // turn off caching so we don't accidentally trigger unnecessary disk activity at this point
    bool useCache = d->cacheTheme;
    d->cacheTheme = false;
    d->setThemeName(themeName, false, false);
    d->cacheTheme = useCache;
    d->fixedName = true;
    connect(d, &ThemePrivate::themeChanged, this, &Theme::themeChanged);
}

Theme::~Theme()
{
    if (d == ThemePrivate::globalTheme) {
        if (!d->ref.deref()) {
            disconnect(ThemePrivate::globalTheme, nullptr, this, nullptr);
            delete ThemePrivate::globalTheme;
            ThemePrivate::globalTheme = nullptr;
            d = nullptr;
        }
    } else {
        if (!d->ref.deref()) {
            delete ThemePrivate::themes.take(d->themeName);
        }
    }
}

void Theme::setBasePath(const QString &basePath)
{
    if (d->basePath == basePath) {
        return;
    }

    d->basePath = basePath;

    d->scheduleThemeChangeNotification(PixmapCache | SvgElementsCache);
}

QString Theme::basePath() const
{
    return d->basePath;
}

void Theme::setSelectors(const QStringList &selectors)
{
    d->selectors = selectors;
    d->scheduleThemeChangeNotification(PixmapCache | SvgElementsCache);
}

QStringList Theme::selectors() const
{
    return d->selectors;
}

void Theme::setThemeName(const QString &themeName)
{
    if (d->themeName == themeName) {
        return;
    }

    if (d != ThemePrivate::globalTheme) {
        disconnect(QCoreApplication::instance(), nullptr, d, nullptr);
        if (!d->ref.deref()) {
            delete ThemePrivate::themes.take(d->themeName);
        }

        auto &priv = ThemePrivate::themes[themeName];
        if (!priv) {
            priv = new ThemePrivate;
            if (QCoreApplication::instance()) {
                connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, priv, &ThemePrivate::onAppExitCleanup);
            }
        }
        priv->ref.ref();
        d = priv;
        connect(d, &ThemePrivate::themeChanged, this, &Theme::themeChanged);
    }

    d->setThemeName(themeName, true, true);
}

QString Theme::themeName() const
{
    return d->themeName;
}

QString Theme::imagePath(const QString &name) const
{
    // look for a compressed svg file in the theme
    if (name.contains(QLatin1String("../")) || name.isEmpty()) {
        // we don't support relative paths
        // qCDebug(LOG_KSVG) << "Theme says: bad image path " << name;
        return QString();
    }

    const QString svgzName = name % QLatin1String(".svgz");
    QString path = d->findInTheme(svgzName, d->themeName);

    if (path.isEmpty()) {
        // try for an uncompressed svg file
        const QString svgName = name % QLatin1String(".svg");
        path = d->findInTheme(svgName, d->themeName);

        // search in fallback themes if necessary
        for (int i = 0; path.isEmpty() && i < d->fallbackThemes.count(); ++i) {
            if (d->themeName == d->fallbackThemes[i]) {
                continue;
            }

            // try a compressed svg file in the fallback theme
            path = d->findInTheme(svgzName, d->fallbackThemes[i]);

            if (path.isEmpty()) {
                // try an uncompressed svg file in the fallback theme
                path = d->findInTheme(svgName, d->fallbackThemes[i]);
            }
        }
    }

    return path;
}

QString Theme::filePath(const QString &name) const
{
    // look for a compressed svg file in the theme
    if (name.contains(QLatin1String("../")) || name.isEmpty()) {
        // we don't support relative paths
        // qCDebug(LOG_KSVG) << "Theme says: bad image path " << name;
        return QString();
    }

    QString path = d->findInTheme(name, d->themeName);

    if (path.isEmpty()) {
        // search in fallback themes if necessary
        for (int i = 0; path.isEmpty() && i < d->fallbackThemes.count(); ++i) {
            if (d->themeName == d->fallbackThemes[i]) {
                continue;
            }

            path = d->findInTheme(name, d->fallbackThemes[i]);
        }
    }

    return path;
}

QPalette Theme::palette() const
{
    return d->palette;
}

QPalette Theme::globalPalette()
{
    if (!ThemePrivate::globalTheme) {
        ThemePrivate::globalTheme = new ThemePrivate;
        ThemePrivate::globalTheme->settingsChanged(false);
    }
    return ThemePrivate::globalTheme->palette;
}

bool Theme::currentThemeHasImage(const QString &name) const
{
    if (name.contains(QLatin1String("../"))) {
        // we don't support relative paths
        return false;
    }

    QString path = d->findInTheme(name % QLatin1String(".svgz"), d->themeName);
    if (path.isEmpty()) {
        path = d->findInTheme(name % QLatin1String(".svg"), d->themeName);
    }
    return path.contains(d->basePath % d->themeName);
}

KSharedConfigPtr Theme::colorScheme() const
{
    return d->colors;
}

void Theme::setUseGlobalSettings(bool useGlobal)
{
    if (d->useGlobal == useGlobal) {
        return;
    }

    d->useGlobal = useGlobal;
    d->cfg = KConfigGroup();
    d->themeName.clear();
    d->settingsChanged(true);
}

bool Theme::useGlobalSettings() const
{
    return d->useGlobal;
}

void Theme::setCacheLimit(int kbytes)
{
    d->cacheSize = kbytes;
    delete d->pixmapCache;
    d->pixmapCache = nullptr;
}

KPluginMetaData Theme::metadata() const
{
    return d->pluginMetaData;
}

}

#include "moc_theme.cpp"
