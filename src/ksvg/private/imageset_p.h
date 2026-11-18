/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_IMAGESET_P_H
#define KSVG_IMAGESET_P_H

#include "imageset.h"
#include "svg.h"
#include <QHash>
#include <QPixmapCache>
#include <QSet>

#include <KColorScheme>
#include <KPluginMetaData>
#include <QDebug>
#include <QTimer>

#include <KConfigGroup>

namespace KSvg
{
class ImageSet;

enum CacheType {
    NoCache = 0,
    PixmapCache = 1,
    SvgElementsCache = 2,
};
Q_DECLARE_FLAGS(CacheTypes, CacheType)
Q_DECLARE_OPERATORS_FOR_FLAGS(CacheTypes)

class ImageSetPrivate : public QObject, public QSharedData
{
    Q_OBJECT

public:
    explicit ImageSetPrivate(const QString &basePath, QObject *parent = nullptr);
    ~ImageSetPrivate() override;

    QString imagePath(const QString &theme, const QString &type, const QString &image);
    QString findInImageSet(const QString &image, const QString &theme, bool cache = true);
    void discardCache(CacheTypes caches);
    void scheduleImageSetChangeNotification(CacheTypes caches);
    bool useCache();
    void setImageSetName(const QString &themeName, bool emitChanged);

    QColor namedColor(Svg::StyleSheetColor colorName, const KSvg::Svg *svg);
    const QString svgStyleSheet(KSvg::Svg *svg);

    /*!
     * Check whether a rendered pixmap is in the process-local cache.
     * \param key the name to use in the cache for this image
     * \param pix the pixmap object to populate with the resulting data if found
     * Returns true when pixmap was found and loaded from cache, false otherwise
     **/
    bool findInCache(const QString &key, QPixmap &pix);

    /*!
     * Insert specified pixmap into the cache.
     * If the cache already contains pixmap with the specified key then it is
     * overwritten.
     *
     * \param key the name to use in the cache for this pixmap
     * \param pix the pixmap data to store in the cache
     **/
    void insertIntoCache(const QString &key, const QPixmap &pix);

    /*!
     * Insert specified pixmap into the cache.
     * If the cache already contains pixmap with the specified key then it is
     * overwritten.
     * The actual insert is delayed for optimization reasons and the id
     * parameter is used to discard repeated inserts in the delay time, useful
     * when for instance the graphics to insert comes from a quickly resizing
     * object: the frames between the start and destination sizes aren't
     * useful in the cache and just cause overhead.
     *
     * \param key the name to use in the cache for this pixmap
     * \param pix the pixmap data to store in the cache
     * \param id a name that identifies the caller class of this function in an unique fashion.
     *           This avoids storing intermediate sizes in the cache.
     *           If an image with the same id changes quickly,
     *           only the last size where insertIntoCache was called is stored.
     **/
    void insertIntoCache(const QString &key, const QPixmap &pix, const QString &id);

    void colorsChanged();

public Q_SLOTS:
    void scheduledCacheUpdate();
    void onAppExitCleanup();
    void notifyOfChanged();

Q_SIGNALS:
    void imageSetChanged(const QString &imageSetName);

public:
    bool eventFilter(QObject *watched, QEvent *event) override;

    static const char defaultImageSet[];
    static const char themeRcFile[];

    // Ref counting of ImageSetPrivate instances
    static ImageSetPrivate *globalImageSet;
    static QHash<QString, ImageSetPrivate *> themes;

    QString imageSetName = QStringLiteral("default");
    QString basePath;
#if KSVG_BUILD_DEPRECATED_SINCE(6, 21)
    KPluginMetaData pluginMetaData;
#endif
    QList<QString> fallbackImageSets;
    KSharedConfigPtr colors;
    KColorScheme colorScheme;
    KColorScheme selectionColorScheme;
    KColorScheme buttonColorScheme;
    KColorScheme viewColorScheme;
    KColorScheme complementaryColorScheme;
    KColorScheme headerColorScheme;
    KColorScheme tooltipColorScheme;
    QStringList selectors;
    KConfigGroup cfg;
    QHash<QString, QPixmap> pixmapsToCache;
    QHash<QString, QString> keysToCache;
    QHash<QString, QString> idsToCache;
    QSet<QString> cachedPixmapKeys;
    QHash<qint64, QString> cachedSvgStyleSheets;
    QHash<qint64, QString> cachedSelectedSvgStyleSheets;
    QHash<qint64, QString> cachedInactiveSvgStyleSheets;
    QHash<QString, QString> discoveries;
    bool hasColorsKnown = false;
    bool hasColors = false;
    QTimer *pixmapSaveTimer;
    QTimer *updateNotificationTimer;
    CacheTypes cachesToDiscard;

#if KSVG_BUILD_DEPRECATED_SINCE(6, 21)
    bool useGlobal : 1;
#endif

    bool cacheImageSet : 1;
};

}

#endif

extern const QString s;
