/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2008-2010 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "svg.h"
#include "framesvg.h"
#include "private/imageset_p.h"
#include "private/svg_p.h"

#include <array>
#include <cmath>
#include <mutex>

#include <QBuffer>
#include <QCoreApplication>
#include <QDir>
#include <QPainter>
#include <QRegularExpression>
#include <QStringBuilder>
#include <QTimeZone>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include <KCompressionDevice>
#include <QDebug>

#include "debug_p.h"
#include "imageset.h"

size_t qHash(const KSvg::SvgPrivate::CacheId &id, size_t seed)
{
    std::array<size_t, 10> parts = {
        ::qHash(id.width),
        ::qHash(id.height),
        ::qHash(id.elementName),
        ::qHash(id.filePath),
        ::qHash(id.status),
        ::qHash(id.scaleFactor),
        ::qHash(id.colorSet),
        ::qHash(id.styleSheet),
        ::qHash(id.extraFlags),
        ::qHash(id.lastModified),
    };
    return qHashRange(parts.begin(), parts.end(), seed);
}

size_t qHash(const QList<QColor> &colors, size_t seed)
{
    std::vector<size_t> parts;
    for (const QColor &c : std::as_const(colors)) {
        parts.push_back(::qHash(c.red()));
        parts.push_back(::qHash(c.green()));
        parts.push_back(::qHash(c.blue()));
        parts.push_back(::qHash(c.alpha()));
    }
    return qHashRange(parts.begin(), parts.end(), seed);
}

namespace KSvg
{
class SvgRectsCacheSingleton
{
public:
    SvgRectsCache self;
};

Q_GLOBAL_STATIC(SvgRectsCacheSingleton, privateSvgRectsCacheSelf)

const size_t SvgRectsCache::s_seed = 0x9e3779b9;

SharedSvgRenderer::SharedSvgRenderer(QObject *parent)
    : QSvgRenderer(parent)
{
}

SharedSvgRenderer::SharedSvgRenderer(const QString &filename, const QString &styleSheet, QHash<QString, QRectF> &interestingElements, QObject *parent)
    : QSvgRenderer(parent)
{
    KCompressionDevice file(filename, KCompressionDevice::GZip);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }
    m_filename = filename;
    m_styleSheet = styleSheet;
    m_interestingElements = interestingElements;
    load(file.readAll(), styleSheet, interestingElements);
}

SharedSvgRenderer::SharedSvgRenderer(const QByteArray &contents, const QString &styleSheet, QHash<QString, QRectF> &interestingElements, QObject *parent)
    : QSvgRenderer(parent)
{
    load(contents, styleSheet, interestingElements);
}

void SharedSvgRenderer::reload()
{
    KCompressionDevice file(m_filename, KCompressionDevice::GZip);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    load(file.readAll(), m_styleSheet, m_interestingElements);
}

bool SharedSvgRenderer::load(const QByteArray &contents, const QString &styleSheet, QHash<QString, QRectF> &interestingElements)
{
    // Apply the style sheet.
    if (!styleSheet.isEmpty() && contents.contains("current-color-scheme")) {
        QByteArray processedContents;
        processedContents.reserve(contents.size());
        QXmlStreamReader reader(contents);

        QBuffer buffer(&processedContents);
        buffer.open(QIODevice::WriteOnly);
        QXmlStreamWriter writer(&buffer);
        bool foundStyleSheet = false;
        while (!reader.atEnd()) {
            reader.readNext();
            if (!foundStyleSheet //
                && reader.tokenType() == QXmlStreamReader::StartElement //
                && reader.qualifiedName() == QLatin1String("style")
                && reader.attributes().value(QLatin1String("id")) == QLatin1String("current-color-scheme")) {
                writer.writeStartElement(QLatin1String("style"));
                writer.writeAttributes(reader.attributes());
                writer.writeCharacters(styleSheet);
                writer.writeEndElement();
                while (reader.tokenType() != QXmlStreamReader::EndElement) {
                    reader.readNext();
                }
                foundStyleSheet = true;
            } else if (reader.tokenType() != QXmlStreamReader::Invalid && !reader.isWhitespace() && !reader.isComment()) {
                writer.writeCurrentToken(reader);
            }
        }
        buffer.close();
        if (!QSvgRenderer::load(processedContents)) {
            return false;
        }
    } else if (!QSvgRenderer::load(contents)) {
        return false;
    }

    // Search the SVG to find and store all ids that contain size hints.
    const QString contentsAsString(QString::fromLatin1(contents));
    static const QRegularExpression idExpr(QLatin1String("id\\s*?=\\s*?(['\"])(\\d+?-\\d+?-.*?)\\1"));
    Q_ASSERT(idExpr.isValid());

    auto matchIt = idExpr.globalMatch(contentsAsString);
    while (matchIt.hasNext()) {
        auto match = matchIt.next();
        QString elementId = match.captured(2);

        QRectF elementRect = boundsOnElement(elementId);
        if (elementRect.isValid()) {
            interestingElements.insert(elementId, elementRect);
        }
    }

    return true;
}

SvgRectsCache::SvgRectsCache(QObject *parent)
    : QObject(parent)
{
}

SvgRectsCache *SvgRectsCache::instance()
{
    return &privateSvgRectsCacheSelf()->self;
}

void SvgRectsCache::insert(KSvg::SvgPrivate::CacheId cacheId, const QRectF &rect)
{
    insert(qHash(cacheId, SvgRectsCache::s_seed), cacheId.filePath, rect);
}

void SvgRectsCache::insert(size_t id, const QString &filePath, const QRectF &rect)
{
    m_localRectCache[filePath].insert(id, rect);
}

bool SvgRectsCache::findElementRect(KSvg::SvgPrivate::CacheId cacheId, QRectF &rect)
{
    return findElementRect(qHash(cacheId, SvgRectsCache::s_seed), cacheId.filePath, rect);
}

bool SvgRectsCache::findElementRect(size_t id, QStringView filePath, QRectF &rect)
{
    const auto cache = m_localRectCache[filePath.toString()];
    const auto it = cache.constFind(id);
    if (it == cache.constEnd()) {
        return false;
    }

    rect = *it;
    return true;
}

bool SvgRectsCache::imageChanged(const QString &path, uint lastModified)
{
    const auto it = m_lastModifiedTimes.constFind(path);
    if (it == m_lastModifiedTimes.constEnd()) {
        m_lastModifiedTimes.insert(path, lastModified);
        return false;
    }

    if (it.value() == lastModified) {
        return false;
    }

    m_lastModifiedTimes.insert(path, lastModified);
    m_localRectCache.remove(path);
    m_sizeHintsForId.remove(path);
    m_naturalSizes.remove(path);
    return true;
}

QList<QSizeF> SvgRectsCache::sizeHintsForId(const QString &path, const QString &id)
{
    return m_sizeHintsForId.value(path).value(id);
}

void SvgRectsCache::insertSizeHintForId(const QString &path, const QString &id, const QSizeF &size)
{
    auto &sizes = m_sizeHintsForId[path][id];
    if (!sizes.contains(size)) {
        sizes.append(size);
    }
}

void SvgRectsCache::setNaturalSize(const QString &path, const QSizeF &size)
{
    m_naturalSizes.insert(path, size);
}

QSizeF SvgRectsCache::naturalSize(const QString &path)
{
    return m_naturalSizes.value(path);
}

SvgPrivate::SvgPrivate(Svg *svg)
    : q(svg)
    , renderer(nullptr)
    , styleCrc(0)
    , lastModified(0)
    , devicePixelRatio(1.0)
    , status(Svg::Status::Normal)
    , multipleImages(false)
    , themed(false)
    , fromCurrentImageSet(false)
    , cacheRendering(true)
    , themeFailed(false)
{
}

SvgPrivate::~SvgPrivate()
{
    eraseRenderer();
}

size_t SvgPrivate::paletteId(const QPalette &palette, const QColor &positive, const QColor &neutral, const QColor &negative) const
{
    std::array<size_t, 4> parts = {
        ::qHash(palette.cacheKey()),
        ::qHash(positive.rgba()),
        ::qHash(neutral.rgba()),
        ::qHash(negative.rgba()),
    };
    return qHashRange(parts.begin(), parts.end(), SvgRectsCache::s_seed);
}

// This function is meant for the rects cache
SvgPrivate::CacheId SvgPrivate::cacheId(QStringView elementId) const
{
    auto idSize = size.isValid() && size != naturalSize ? size : QSizeF{-1.0, -1.0};
    return CacheId{idSize.width(), idSize.height(), path, elementId.toString(), status, devicePixelRatio, -1, 0, 0, lastModified};
}

// This function is meant for the pixmap cache
QString SvgPrivate::cachePath(const QString &id, const QSize &size) const
{
    std::vector<size_t> parts;
    const auto colors = colorOverrides.values();
    for (const QColor &c : std::as_const(colors)) {
        parts.push_back(::qHash(c.red()));
        parts.push_back(::qHash(c.green()));
        parts.push_back(::qHash(c.blue()));
        parts.push_back(::qHash(c.alpha()));
    }
    const size_t colorsHash = qHashRange(parts.begin(), parts.end(), SvgRectsCache::s_seed);

    auto cacheId = CacheId{double(size.width()), double(size.height()), path, id, status, devicePixelRatio, colorSet, colorsHash, 0, lastModified};
    return QString::number(qHash(cacheId, SvgRectsCache::s_seed));
}

bool SvgPrivate::setImagePath(const QString &imagePath)
{
    QString actualPath = imagePath;
    bool isAbsoluteFile = QDir::isAbsolutePath(actualPath);
    if (imagePath.startsWith(QLatin1String("file://"))) {
        // length of file://
        actualPath.remove(0, 7);
        isAbsoluteFile = true;
    }
    // If someone using the QML API uses Qt.resolvedUrl to get the absolute path inside of a QRC,
    // the URI will look something like qrc:/artwork/file.svg
    // In order for file IO to work it needs to be reformatted it needs to be :/artwork/file.svg
    if (imagePath.startsWith(QLatin1String("qrc:/"))) {
        actualPath.replace(QLatin1String("qrc:/"), QLatin1String(":/"));
        isAbsoluteFile = true;
    }

    bool isThemed = !actualPath.isEmpty() && !isAbsoluteFile;

    // lets check to see if we're already set to this file
    if (isThemed == themed && ((themed && themePath == actualPath) || (!themed && path == actualPath))) {
        return false;
    }

    eraseRenderer();

    // if we don't have any path right now and are going to set one,
    // then lets not schedule a repaint because we are just initializing!
    bool updateNeeded = true; //! path.isEmpty() || !themePath.isEmpty();

    QObject::disconnect(imageSetChangedConnection);

    themed = isThemed;
    path.clear();
    themePath.clear();

    bool oldfromCurrentImageSet = fromCurrentImageSet;
    fromCurrentImageSet = isThemed && actualImageSet()->currentImageSetHasImage(imagePath);

    if (fromCurrentImageSet != oldfromCurrentImageSet) {
        Q_EMIT q->fromCurrentImageSetChanged(fromCurrentImageSet);
    }

    if (themed) {
        themePath = actualPath;
        path = actualImageSet()->imagePath(themePath);
        themeFailed = path.isEmpty();
        imageSetChangedConnection = QObject::connect(actualImageSet(), &ImageSet::imageSetChanged, q, [this]() {
            imageSetChanged();
        });
    } else if (QFileInfo::exists(actualPath)) {
        imageSetChangedConnection = QObject::connect(actualImageSet(), &ImageSet::imageSetChanged, q, [this]() {
            imageSetChanged();
        });
        path = actualPath;
    } else {
#ifndef NDEBUG
        // qCDebug(LOG_KSVG) << "file '" << path << "' does not exist!";
#endif
    }

    QDateTime lastModifiedDate;
    if (!path.isEmpty()) {
        const QFileInfo info(path);
        lastModifiedDate = info.lastModified(QTimeZone::utc());

        lastModified = lastModifiedDate.toSecsSinceEpoch();

        const bool imageChanged = SvgRectsCache::instance()->imageChanged(path, lastModified);

        if (imageChanged) {
            std::unique_lock lock(s_renderersLock);
            auto i = s_renderers.constBegin();
            while (i != s_renderers.constEnd()) {
                if (i.key().contains(path)) {
                    i.value()->reload();
                }
                i++;
            }
        }
    }

    // also images with absolute path needs to have a natural size initialized,
    // even if looks a bit weird using ImageSet to store non-themed stuff
    if ((themed && !path.isEmpty() && lastModifiedDate.isValid()) || QFileInfo::exists(actualPath)) {
        naturalSize = SvgRectsCache::instance()->naturalSize(path);
        if (naturalSize.isEmpty()) {
            createRenderer();
            naturalSize = renderer->defaultSize();
            SvgRectsCache::instance()->setNaturalSize(path, naturalSize);
        }
    }

    q->resize();
    Q_EMIT q->imagePathChanged();

    return updateNeeded;
}

ImageSet *SvgPrivate::actualImageSet()
{
    if (!theme) {
        theme = new KSvg::ImageSet(q);
    }

    return theme.data();
}

QPixmap SvgPrivate::findInCache(const QString &elementId, qreal ratio, const QSizeF &s)
{
    QSize size;
    QString actualElementId;

    // Look at the size hinted elements and try to find the smallest one with an
    // identical aspect ratio.
    if (s.isValid() && !elementId.isEmpty()) {
        const QList<QSizeF> elementSizeHints = SvgRectsCache::instance()->sizeHintsForId(path, elementId);

        if (!elementSizeHints.isEmpty()) {
            QSizeF bestFit(-1, -1);

            for (const auto &hint : elementSizeHints) {
                if (hint.width() >= s.width() * ratio && hint.height() >= s.height() * ratio
                    && (!bestFit.isValid() || (bestFit.width() * bestFit.height()) > (hint.width() * hint.height()))) {
                    bestFit = hint;
                }
            }

            if (bestFit.isValid()) {
                actualElementId = QString::number(bestFit.width()) % QLatin1Char('-') % QString::number(bestFit.height()) % QLatin1Char('-') % elementId;
            }
        }
    }

    if (elementId.isEmpty() || !q->hasElement(actualElementId)) {
        actualElementId = elementId;
    }

    if (elementId.isEmpty() || (multipleImages && s.isValid())) {
        size = s.toSize() * ratio;
    } else {
        size = elementRect(actualElementId).size().toSize() * ratio;
    }

    if (size.isEmpty()) {
        return QPixmap();
    }

    const QString id = cachePath(actualElementId, size);

    QPixmap p;
    if (cacheRendering && actualImageSet()->d->findInCache(id, p)) {
        p.setDevicePixelRatio(ratio);
        // qCDebug(LOG_PLASMA) << "found cached version of " << id << p.size();
        return p;
    }

    createRenderer();

    QRectF finalRect = makeUniform(renderer->boundsOnElement(actualElementId), QRect(QPoint(0, 0), size));

    // don't alter the pixmap size or it won't match up properly to, e.g., FrameSvg elements
    // makeUniform should never change the size so much that it gains or loses a whole pixel
    p = QPixmap(size);

    p.fill(Qt::transparent);
    QPainter renderPainter(&p);

    if (actualElementId.isEmpty()) {
        renderer->render(&renderPainter, finalRect);
    } else {
        renderer->render(&renderPainter, actualElementId, finalRect);
    }

    renderPainter.end();
    p.setDevicePixelRatio(ratio);

    if (cacheRendering) {
        actualImageSet()->d->insertIntoCache(id, p, QString::number((qint64)q, 16) % QLatin1Char('_') % actualElementId);
    }

    return p;
}

void SvgPrivate::createRenderer()
{
    if (renderer) {
        return;
    }

    if (themed && path.isEmpty() && !themeFailed) {
        if (path.isEmpty()) {
            path = actualImageSet()->imagePath(themePath);
            themeFailed = path.isEmpty();
            if (themeFailed) {
                qCWarning(LOG_KSVG) << "No image path found for" << themePath;
            }
        }
    }

    QString styleSheet;
    if (!colorOverrides.isEmpty()) {
        if (stylesheetOverride.isEmpty()) {
            stylesheetOverride = actualImageSet()->d->svgStyleSheet(q);
        }
        styleSheet = stylesheetOverride;
    } else {
        styleSheet = actualImageSet()->d->svgStyleSheet(q);
    }

    styleCrc = qChecksum(QByteArrayView(styleSheet.toUtf8().constData(), styleSheet.size()));

    // If the KSVG lives on the main thread (the majority case) share a renderer
    // For other uses we should not as QSvgRenderer is not thread safe.
    // We cannot compare the current thread as this might be hit from the render thread in updatePaintNode for KSVG objects living on the main thread
    if (q->thread() == qGuiApp->thread()) {
        std::shared_lock lock(s_renderersLock);
        QHash<QString, SharedSvgRenderer::Ptr>::const_iterator it = s_renderers.constFind(styleCrc + path);

        if (it != s_renderers.constEnd()) {
            renderer = it.value();
            if (size == QSizeF()) {
                size = renderer->defaultSize();
            }
            return;
        }
    }

    if (path.isEmpty()) {
        renderer = new SharedSvgRenderer();
    } else {
        QHash<QString, QRectF> interestingElements;
        renderer = new SharedSvgRenderer(path, styleSheet, interestingElements);

        // Add interesting elements to the theme's rect cache.
        QHashIterator<QString, QRectF> i(interestingElements);

        QRegularExpression sizeHintedKeyExpr(QStringLiteral("^(\\d+)-(\\d+)-(.+)$"));

        while (i.hasNext()) {
            i.next();
            const QString &elementId = i.key();
            QString originalId = i.key();
            const QRectF &elementRect = i.value();

            originalId.replace(sizeHintedKeyExpr, QStringLiteral("\\3"));
            SvgRectsCache::instance()->insertSizeHintForId(path, originalId, elementRect.size().toSize());

            const CacheId cacheId{.width = -1.0,
                                  .height = -1.0,
                                  .filePath = path,
                                  .elementName = elementId,
                                  .status = status,
                                  .scaleFactor = devicePixelRatio,
                                  .colorSet = -1,
                                  .styleSheet = 0,
                                  .extraFlags = 0,
                                  .lastModified = lastModified};
            SvgRectsCache::instance()->insert(cacheId, elementRect);
        }
    }

    {
        std::unique_lock lock(s_renderersLock);
        s_renderers[styleCrc + path] = renderer;
        if (size == QSizeF()) {
            size = renderer->defaultSize();
        }
    }
}

void SvgPrivate::eraseRenderer()
{
    if (renderer && renderer->ref.loadRelaxed() == 2) {
        // this and the cache reference it
        std::unique_lock lock(s_renderersLock);
        s_renderers.erase(s_renderers.find(styleCrc + path));
    }

    renderer = nullptr;
    styleCrc = QChar(0);
}

QRectF SvgPrivate::elementRect(QStringView elementId)
{
    if (themed && path.isEmpty()) {
        if (themeFailed) {
            return QRectF();
        }

        path = actualImageSet()->imagePath(themePath);
        themeFailed = path.isEmpty();

        if (themeFailed) {
            return QRectF();
        }
    }

    if (path.isEmpty()) {
        return QRectF();
    }

    QRectF rect;
    const CacheId cacheId = SvgPrivate::cacheId(elementId);
    bool found = SvgRectsCache::instance()->findElementRect(cacheId, rect);
    // This is a corner case where we are *sure* the element is not valid
    if (!found) {
        rect = findAndCacheElementRect(elementId);
    }

    return rect;
}

QRectF SvgPrivate::findAndCacheElementRect(QStringView elementId)
{
    // we need to check the id before createRenderer(), otherwise it may generate a different id compared to the previous cacheId)( call
    const CacheId cacheId = SvgPrivate::cacheId(elementId);

    createRenderer();

    auto elementIdString = elementId.toString();

    // This code will usually never be run because createRenderer already caches all the boundingRect in the elements in the svg
    QRectF elementRect = renderer->elementExists(elementIdString)
        ? renderer->transformForElement(elementIdString).map(renderer->boundsOnElement(elementIdString)).boundingRect()
        : QRectF();

    naturalSize = renderer->defaultSize();

    qreal dx = size.width() / renderer->defaultSize().width();
    qreal dy = size.height() / renderer->defaultSize().height();

    elementRect = QRectF(elementRect.x() * dx, elementRect.y() * dy, elementRect.width() * dx, elementRect.height() * dy);
    SvgRectsCache::instance()->insert(cacheId, elementRect);

    return elementRect;
}

bool Svg::eventFilter(QObject *watched, QEvent *event)
{
    return QObject::eventFilter(watched, event);
}

// Following two are utility functions to snap rendered elements to the pixel grid
// to and from are always 0 <= val <= 1
qreal SvgPrivate::closestDistance(qreal to, qreal from)
{
    qreal a = to - from;
    if (qFuzzyCompare(to, from)) {
        return 0;
    } else if (to > from) {
        qreal b = to - from - 1;
        return (qAbs(a) > qAbs(b)) ? b : a;
    } else {
        qreal b = 1 + to - from;
        return (qAbs(a) > qAbs(b)) ? b : a;
    }
}

QRectF SvgPrivate::makeUniform(const QRectF &orig, const QRectF &dst)
{
    if (qFuzzyIsNull(orig.x()) || qFuzzyIsNull(orig.y())) {
        return dst;
    }

    QRectF res(dst);
    qreal div_w = dst.width() / orig.width();
    qreal div_h = dst.height() / orig.height();

    qreal div_x = dst.x() / orig.x();
    qreal div_y = dst.y() / orig.y();

    // horizontal snap
    if (!qFuzzyIsNull(div_x) && !qFuzzyCompare(div_w, div_x)) {
        qreal rem_orig = orig.x() - (floor(orig.x()));
        qreal rem_dst = dst.x() - (floor(dst.x()));
        qreal offset = closestDistance(rem_dst, rem_orig);
        res.translate(offset + offset * div_w, 0);
        res.setWidth(res.width() + offset);
    }
    // vertical snap
    if (!qFuzzyIsNull(div_y) && !qFuzzyCompare(div_h, div_y)) {
        qreal rem_orig = orig.y() - (floor(orig.y()));
        qreal rem_dst = dst.y() - (floor(dst.y()));
        qreal offset = closestDistance(rem_dst, rem_orig);
        res.translate(0, offset + offset * div_h);
        res.setHeight(res.height() + offset);
    }

    return res;
}

void SvgPrivate::imageSetChanged()
{
    if (q->imagePath().isEmpty()) {
        return;
    }

    QString currentPath = themed ? themePath : path;
    themePath.clear();
    // If imageset colors changed, stylesheetOverride will lways be
    // invalid, because it contains both the colors it overrides and the
    // colors it doesn't, which come from imageset colors
    stylesheetOverride.clear();
    eraseRenderer();
    setImagePath(currentPath);
    q->resize();

    // qCDebug(LOG_KSVG) << themePath << ">>>>>>>>>>>>>>>>>> theme changed";
    Q_EMIT q->repaintNeeded();
    Q_EMIT q->imageSetChanged(q->imageSet());
}

void SvgPrivate::colorsChanged()
{
    eraseRenderer();
    qCDebug(LOG_KSVG) << "repaint needed from colorsChanged";

    Q_EMIT q->repaintNeeded();
}

std::shared_mutex SvgPrivate::s_renderersLock;
QHash<QString, SharedSvgRenderer::Ptr> SvgPrivate::s_renderers;
QPointer<ImageSet> SvgPrivate::s_systemColorsCache;

Svg::Svg(QObject *parent)
    : QObject(parent)
    , d(new SvgPrivate(this))
{
}

Svg::~Svg()
{
    delete d;
}

void Svg::setDevicePixelRatio(qreal ratio)
{
    if (FrameSvg *f = qobject_cast<FrameSvg *>(this)) {
        f->clearCache();
    }

    d->devicePixelRatio = ratio;

    Q_EMIT repaintNeeded();
}

qreal Svg::devicePixelRatio() const
{
    return d->devicePixelRatio;
}

QPixmap Svg::pixmap(const QString &elementID)
{
    if (elementID.isNull() || d->multipleImages) {
        return d->findInCache(elementID, d->devicePixelRatio, size());
    } else {
        return d->findInCache(elementID, d->devicePixelRatio);
    }
}

QImage Svg::image(const QSize &size, const QString &elementID)
{
    QPixmap pix(d->findInCache(elementID, d->devicePixelRatio, size));
    return pix.toImage();
}

void Svg::paint(QPainter *painter, const QPointF &point, const QString &elementID)
{
    Q_ASSERT(painter->device());
    const qreal ratio = painter->device()->devicePixelRatio();
    QPixmap pix((elementID.isNull() || d->multipleImages) ? d->findInCache(elementID, ratio, size()) : d->findInCache(elementID, ratio));

    if (pix.isNull()) {
        return;
    }

    painter->drawPixmap(QRectF(point, size()), pix, QRectF(QPointF(0, 0), pix.size()));
}

void Svg::paint(QPainter *painter, int x, int y, const QString &elementID)
{
    paint(painter, QPointF(x, y), elementID);
}

void Svg::paint(QPainter *painter, const QRectF &rect, const QString &elementID)
{
    Q_ASSERT(painter->device());
    const qreal ratio = painter->device()->devicePixelRatio();
    QPixmap pix(d->findInCache(elementID, ratio, rect.size()));

    painter->drawPixmap(rect, pix, QRect(QPoint(0, 0), pix.size()));
}

void Svg::paint(QPainter *painter, int x, int y, int width, int height, const QString &elementID)
{
    Q_ASSERT(painter->device());
    const qreal ratio = painter->device()->devicePixelRatio();
    QPixmap pix(d->findInCache(elementID, ratio, QSizeF(width, height)));
    painter->drawPixmap(x, y, pix, 0, 0, pix.size().width(), pix.size().height());
}

QSizeF Svg::size() const
{
    if (d->size.isEmpty()) {
        d->size = d->naturalSize;
    }

    return {std::round(d->size.width()), std::round(d->size.height())};
}

void Svg::resize(qreal width, qreal height)
{
    resize(QSize(width, height));
}

void Svg::resize(const QSizeF &size)
{
    if (qFuzzyCompare(size.width(), d->size.width()) && qFuzzyCompare(size.height(), d->size.height())) {
        return;
    }

    d->size = size;
    Q_EMIT sizeChanged();
}

void Svg::resize()
{
    if (qFuzzyCompare(d->naturalSize.width(), d->size.width()) && qFuzzyCompare(d->naturalSize.height(), d->size.height())) {
        return;
    }

    d->size = d->naturalSize;
    Q_EMIT sizeChanged();
}

QSizeF Svg::elementSize(const QString &elementId) const
{
    const QSizeF s = d->elementRect(elementId).size();
    return {std::round(s.width()), std::round(s.height())};
}

QSizeF Svg::elementSize(QStringView elementId) const
{
    const QSizeF s = d->elementRect(elementId).size();
    return {std::round(s.width()), std::round(s.height())};
}

QRectF Svg::elementRect(const QString &elementId) const
{
    return d->elementRect(elementId);
}

QRectF Svg::elementRect(QStringView elementId) const
{
    return d->elementRect(elementId);
}

bool Svg::hasElement(const QString &elementId) const
{
    return hasElement(QStringView(elementId));
}

bool Svg::hasElement(QStringView elementId) const
{
    if (elementId.isEmpty() || (d->path.isNull() && d->themePath.isNull())) {
        return false;
    }

    return d->elementRect(elementId).isValid();
}

bool Svg::isValid() const
{
    if (d->path.isNull() && d->themePath.isNull()) {
        return false;
    }

    // try very hard to avoid creation of a parser
    QSizeF naturalSize = SvgRectsCache::instance()->naturalSize(d->path);
    if (!naturalSize.isEmpty()) {
        return true;
    }

    if (d->path.isEmpty() || !QFileInfo::exists(d->path)) {
        return false;
    }
    d->createRenderer();
    return d->renderer->isValid();
}

void Svg::setContainsMultipleImages(bool multiple)
{
    d->multipleImages = multiple;
}

bool Svg::containsMultipleImages() const
{
    return d->multipleImages;
}

void Svg::setImagePath(const QString &svgFilePath)
{
    if (d->setImagePath(svgFilePath)) {
        Q_EMIT repaintNeeded();
    }
}

QString Svg::imagePath() const
{
    return d->themed ? d->themePath : d->path;
}

void Svg::setUsingRenderingCache(bool useCache)
{
    d->cacheRendering = useCache;
    Q_EMIT repaintNeeded();
}

bool Svg::isUsingRenderingCache() const
{
    return d->cacheRendering;
}

bool Svg::fromCurrentImageSet() const
{
    return d->fromCurrentImageSet;
}

void Svg::setImageSet(KSvg::ImageSet *theme)
{
    if (!theme || theme == d->theme.data()) {
        return;
    }

    if (d->theme) {
        disconnect(d->theme.data(), nullptr, this, nullptr);
    }

    d->theme = theme;
    connect(theme, SIGNAL(imageSetChanged(QString)), this, SLOT(imageSetChanged()));
    d->imageSetChanged();
}

ImageSet *Svg::imageSet() const
{
    return d->actualImageSet();
}

void Svg::setStatus(KSvg::Svg::Status status)
{
    if (status == d->status) {
        return;
    }

    d->status = status;
    d->eraseRenderer();
    Q_EMIT statusChanged(status);
    Q_EMIT repaintNeeded();
}

Svg::Status Svg::status() const
{
    return d->status;
}

void Svg::setColorSet(KSvg::Svg::ColorSet colorSet)
{
    const KColorScheme::ColorSet convertedSet = KColorScheme::ColorSet(colorSet);
    if (convertedSet == d->colorSet) {
        return;
    }

    d->colorSet = convertedSet;
    d->eraseRenderer();
    Q_EMIT colorSetChanged(colorSet);
    Q_EMIT repaintNeeded();
}

Svg::ColorSet Svg::colorSet() const
{
    return Svg::ColorSet(d->colorSet);
}

QColor Svg::color(StyleSheetColor colorName) const
{
    auto it = d->colorOverrides.constFind(colorName);
    if (it != d->colorOverrides.constEnd()) {
        return *it;
    }
    return d->actualImageSet()->d->namedColor(colorName, this);
}

void Svg::setColor(StyleSheetColor colorName, const QColor &color)
{
    setColors({
        {colorName, color},
    });
}

void Svg::setColors(const QMap<StyleSheetColor, QColor> &colors)
{
    bool changed = false;
    for (const auto &[colorName, color] : colors.asKeyValueRange()) {
        if (d->colorOverrides.value(colorName) != color) {
            changed = true;

            if (color.isValid()) {
                d->colorOverrides[colorName] = color;
            } else {
                d->colorOverrides.remove(colorName);
            }
        }
    }

    if (!changed) {
        return;
    }

    d->stylesheetOverride.clear();

    d->eraseRenderer();

    // Ideally, there should be a signal but adding a new signal can make ksvg users crash in QML cache code.
    if (auto frameSvg = qobject_cast<FrameSvg *>(this)) {
        frameSvg->colorOverridesChange();
    }

    Q_EMIT repaintNeeded();
}

QMap<Svg::StyleSheetColor, QColor> Svg::colorOverrides() const
{
    return d->colorOverrides;
}

void Svg::clearColorOverrides()
{
    d->colorOverrides.clear();
    d->stylesheetOverride.clear();
    d->eraseRenderer();
    if (auto frameSvg = qobject_cast<FrameSvg *>(this)) {
        frameSvg->colorOverridesChange();
    }
    Q_EMIT repaintNeeded();
}

} // KSvg namespace

#include "moc_svg.cpp"
#include "private/moc_svg_p.cpp"
