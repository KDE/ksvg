/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_THEME_P_H
#define KSVG_THEME_P_H

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

    const QString processStyleSheet(const QString &css, KSvg::Svg::Status status);
    const QString svgStyleSheet(KSvg::Theme::ColorGroup group, KSvg::Svg::Status status);
    QColor color(Theme::ColorRole role, Theme::ColorGroup group = Theme::NormalColorGroup) const;

    /**
     * TODO: timestamp shouldn't be user-provided
     * Check with file timestamp
     * where cache is still valid.
     *
     * @param key the name to use in the cache for this image
     * @param pix the pixmap object to populate with the resulting data if found
     * @param lastModified if non-zero, the time stamp is also checked on the file,
     *                     and must be newer than the timestamp to be loaded
     *
     * @note Since KF 5.75, a lastModified value of 0 is deprecated. If used, it
     *       will now always return false. Use a proper file timestamp instead
     *       so modification can be properly tracked.
     *
     * @return true when pixmap was found and loaded from cache, false otherwise
     **/
    bool findInCache(const QString &key, QPixmap &pix, unsigned int lastModified = 0);

    /**
     * Insert specified pixmap into the cache.
     * If the cache already contains pixmap with the specified key then it is
     * overwritten.
     *
     * @param key the name to use in the cache for this pixmap
     * @param pix the pixmap data to store in the cache
     **/
    void insertIntoCache(const QString &key, const QPixmap &pix);

    /**
     * Insert specified pixmap into the cache.
     * If the cache already contains pixmap with the specified key then it is
     * overwritten.
     * The actual insert is delayed for optimization reasons and the id
     * parameter is used to discard repeated inserts in the delay time, useful
     * when for instance the graphics to insert comes from a quickly resizing
     * object: the frames between the start and destination sizes aren't
     * useful in the cache and just cause overhead.
     *
     * @param key the name to use in the cache for this pixmap
     * @param pix the pixmap data to store in the cache
     * @param id a name that identifies the caller class of this function in an unique fashion.
     *           This is needed to limit disk writes of the cache.
     *           If an image with the same id changes quickly,
     *           only the last size where insertIntoCache was called is actually stored on disk
     **/
    void insertIntoCache(const QString &key, const QPixmap &pix, const QString &id);

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
