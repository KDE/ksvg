/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA_THEME_P_H
#define PLASMA_THEME_P_H

#include "svg.h"
#include "theme.h"
#include <QHash>

#include <KColorScheme>
#include <KImageCache>
#include <KPluginMetaData>
#include <KSharedDataCache>
#include <QDebug>
#include <QTimer>

#include "libplasma-theme-global.h"

namespace KSvg
{
class Theme;

enum CacheType {
    NoCache = 0,
    PixmapCache = 1,
    SvgElementsCache = 2,
};
Q_DECLARE_FLAGS(CacheTypes, CacheType)
Q_DECLARE_OPERATORS_FOR_FLAGS(CacheTypes)

class ThemePrivate : public QObject, public QSharedData
{
    Q_OBJECT

public:
    explicit ThemePrivate(QObject *parent = nullptr);
    ~ThemePrivate() override;

    KConfigGroup &config();

    QString imagePath(const QString &theme, const QString &type, const QString &image);
    QString findInTheme(const QString &image, const QString &theme, bool cache = true);
    void discardCache(CacheTypes caches);
    void scheduleThemeChangeNotification(CacheTypes caches);
    bool useCache();
    void setThemeName(const QString &themeName, bool writeSettings, bool emitChanged);

    const QString svgStyleSheet(KSvg::Theme::ColorGroup group, KSvg::Svg::Status status);
    QColor color(Theme::ColorRole role, Theme::ColorGroup group = Theme::NormalColorGroup) const;

public Q_SLOTS:
    void colorsChanged();
    void settingsFileChanged(const QString &settings);
    void scheduledCacheUpdate();
    void onAppExitCleanup();
    void notifyOfChanged();
    void settingsChanged(bool emitChanges);

Q_SIGNALS:
    void themeChanged();
    void applicationPaletteChange();

public:
    static const char defaultTheme[];
    static const char systemColorsTheme[];
    static const char themeRcFile[];

    // Ref counting of ThemePrivate instances
    static ThemePrivate *globalTheme;
    static QHash<QString, ThemePrivate *> themes;

    QString themeName;
    QString basePath;
    KPluginMetaData pluginMetaData;
    QList<QString> fallbackThemes;
    QStringList selectors;
    KSharedConfigPtr colors;
    KColorScheme colorScheme;
    KColorScheme selectionColorScheme;
    KColorScheme buttonColorScheme;
    KColorScheme viewColorScheme;
    KColorScheme complementaryColorScheme;
    KColorScheme headerColorScheme;
    KColorScheme tooltipColorScheme;
    QPalette palette;
    bool eventFilter(QObject *watched, QEvent *event) override;
    KConfigGroup cfg;
    KImageCache *pixmapCache;
    QHash<QString, QPixmap> pixmapsToCache;
    QHash<QString, QString> keysToCache;
    QHash<QString, QString> idsToCache;
    QHash<Theme::ColorGroup, QString> cachedSvgStyleSheets;
    QHash<Theme::ColorGroup, QString> cachedSelectedSvgStyleSheets;
    QHash<Theme::ColorGroup, QString> cachedInactiveSvgStyleSheets;
    QHash<QString, QString> discoveries;
    QTimer *pixmapSaveTimer;
    QTimer *updateNotificationTimer;
    unsigned cacheSize;
    CacheTypes cachesToDiscard;
    QString themeVersion;
    QString themeMetadataPath;
    QString iconThemeMetadataPath;

    bool isDefault : 1;
    bool useGlobal : 1;
    bool cacheTheme : 1;
    bool fixedName : 1;

    // Version number of Plasma the Theme has been designed for
    int apiMajor;
    int apiMinor;
    int apiRevision;
};

}

#endif

extern const QString s;
