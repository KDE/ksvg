/*
    SPDX-FileCopyrightText: 2006-2010 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_SVG_P_H
#define KSVG_SVG_P_H

#include "svg.h"

#include <KColorScheme>

#include <QExplicitlySharedDataPointer>
#include <QHash>
#include <QObject>
#include <QPalette>
#include <QPointer>
#include <QSharedData>
#include <QSvgRenderer>

namespace KSvg
{
class SharedSvgRenderer : public QSvgRenderer, public QSharedData
{
    Q_OBJECT
public:
    typedef QExplicitlySharedDataPointer<SharedSvgRenderer> Ptr;

    explicit SharedSvgRenderer(QObject *parent = nullptr);
    SharedSvgRenderer(const QString &filename, const QString &styleSheet, QHash<QString, QRectF> &interestingElements, QObject *parent = nullptr);

    SharedSvgRenderer(const QByteArray &contents, const QString &styleSheet, QHash<QString, QRectF> &interestingElements, QObject *parent = nullptr);

    void reload();

private:
    bool load(const QByteArray &contents, const QString &styleSheet, QHash<QString, QRectF> &interestingElements);

    QString m_filename;
    QString m_styleSheet;
    QHash<QString, QRectF> m_interestingElements;
};

class SvgPrivate
{
public:
    struct CacheId {
        double width;
        double height;
        QString filePath;
        QString elementName;
        int status;
        double scaleFactor;
        int colorSet;
        uint styleSheet;
        uint extraFlags; // Not used here, used for enabledborders in FrameSvg
        uint lastModified;
    };

    SvgPrivate(Svg *svg);
    ~SvgPrivate();

    qint64 paletteId(const QPalette &palette, const QColor &positive, const QColor &neutral, const QColor &negative) const;

    // This function is meant for the rects cache
    CacheId cacheId(QStringView elementId) const;

    // This function is meant for the pixmap cache
    QString cachePath(const QString &path, const QSize &size) const;

    bool setImagePath(const QString &imagePath);

    ImageSet *actualImageSet();
    ImageSet *cacheAndColorsImageSet();

    QPixmap findInCache(const QString &elementId, const QSizeF &s = QSizeF());

    void createRenderer();
    void eraseRenderer();

    QRectF elementRect(QStringView elementId);
    QRectF findAndCacheElementRect(QStringView elementId);

    // Following two are utility functions to snap rendered elements to the pixel grid
    // to and from are always 0 <= val <= 1
    qreal closestDistance(qreal to, qreal from);

    QRectF makeUniform(const QRectF &orig, const QRectF &dst);

    // Slots
    void imageSetChanged();
    void colorsChanged();

    static QHash<QString, SharedSvgRenderer::Ptr> s_renderers;
    static QPointer<ImageSet> s_systemColorsCache;
    static qreal s_lastScaleFactor;

    Svg *q;
    QPointer<ImageSet> theme;
    SharedSvgRenderer::Ptr renderer;
    QString themePath;
    QString path;
    QSizeF size;
    QSizeF naturalSize;
    QChar styleCrc;
    // We need colorOverrides.values() to have a stable order
    QMap<Svg::StyleSheetColor, QColor> colorOverrides;
    QString stylesheetOverride;
    KColorScheme::ColorSet colorSet = KColorScheme::Window;
    unsigned int lastModified;
    qreal scaleFactor;
    Svg::Status status;

    bool multipleImages : 1;
    bool themed : 1;
    bool useSystemColors : 1;
    bool fromCurrentImageSet : 1;
    bool cacheRendering : 1;
    bool themeFailed : 1;
};

class SvgRectsCache : public QObject
{
    Q_OBJECT
public:
    SvgRectsCache(QObject *parent = nullptr);

    static SvgRectsCache *instance();

    void insert(SvgPrivate::CacheId cacheId, const QRectF &rect, unsigned int lastModified);
    void insert(uint id, const QString &filePath, const QRectF &rect, unsigned int lastModified);
    // Those 2 methods are the same, the second uses the integer id produced by hashed CacheId
    bool findElementRect(SvgPrivate::CacheId cacheId, QRectF &rect);
    bool findElementRect(uint id, QStringView filePath, QRectF &rect);

    bool loadImageFromCache(const QString &path, uint lastModified);
    void dropImageFromCache(const QString &path);

    void setNaturalSize(const QString &path, qreal scaleFactor, const QSizeF &size);
    QSizeF naturalSize(const QString &path, qreal scaleFactor);

    QList<QSize> sizeHintsForId(const QString &path, const QString &id);
    void insertSizeHintForId(const QString &path, const QString &id, const QSize &size);

    QString iconThemePath();
    void setIconThemePath(const QString &path);

    QStringList cachedKeysForPath(const QString &path) const;

    unsigned int lastModifiedTimeFromCache(const QString &filePath);

    void updateLastModified(const QString &filePath, unsigned int lastModified);

    static const uint s_seed;

Q_SIGNALS:
    void lastModifiedChanged(const QString &filePath, unsigned int lastModified);

private:
    QTimer *m_configSyncTimer = nullptr;
    QString m_iconThemePath;
    KSharedConfigPtr m_svgElementsCache;
    /*
     * We are indexing in the hash cache ids by their "digested" uint out of qHash(CacheId)
     * because we need to serialize it and unserialize it to a config file,
     * which is more efficient to do that with the uint directly rather than a CacheId struct serialization
     */
    QHash<uint, QRectF> m_localRectCache;
    QHash<QString, QSet<unsigned int>> m_invalidElements;
    QHash<QString, QList<QSize>> m_sizeHintsForId;
    QHash<QString, unsigned int> m_lastModifiedTimes;
};
}

uint qHash(const KSvg::SvgPrivate::CacheId &id, uint seed = 0);

#endif
