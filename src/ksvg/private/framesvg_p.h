/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2009 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_FRAMESVG_P_H
#define KSVG_FRAMESVG_P_H

#include <QCache>
#include <QHash>
#include <QImage>
#include <QStringBuilder>

#include <algorithm>

#include <QDebug>

#include <KSvg/ImageSet>

#include "framesvg.h"
#include "svg_p.h"

namespace KSvg
{
class FrameData
{
public:
    FrameData(FrameSvg *svg, const QString &p)
        : imagePath(svg->imagePath())
        , prefix(p)
        , enabledBorders(FrameSvg::AllBorders)
        , frameSize(-1, -1)
        , topHeight(0)
        , leftWidth(0)
        , rightWidth(0)
        , bottomHeight(0)
        , topMargin(0)
        , leftMargin(0)
        , rightMargin(0)
        , bottomMargin(0)
        , noBorderPadding(false)
        , stretchBorders(false)
        , tileCenter(false)
        , composeOverBorder(false)
        , centerSolidValid(false)
        , centerIsSolid(false)
        , imageSet(nullptr)
    {
    }

    FrameData(const FrameData &other)
        : imagePath(other.imagePath)
        , prefix(other.prefix)
        , enabledBorders(other.enabledBorders)
        , cachedMasks(MAX_CACHED_MASKS)
        , frameSize(other.frameSize)
        , topHeight(0)
        , leftWidth(0)
        , rightWidth(0)
        , bottomHeight(0)
        , topMargin(0)
        , leftMargin(0)
        , rightMargin(0)
        , bottomMargin(0)
        , noBorderPadding(false)
        , stretchBorders(false)
        , tileCenter(false)
        , composeOverBorder(false)
        , centerSolidValid(false)
        , centerIsSolid(false)
        , imageSet(nullptr)
    {
    }

    ~FrameData();

    QString imagePath;
    QString prefix;
    QString requestedPrefix;
    int colorSet = 0;
    QMap<Svg::StyleSheetColor, QColor> colorOverrides;
    FrameSvg::EnabledBorders enabledBorders;
    QPixmap cachedBackground;
    // Cached result of the "center" element uniformity check, valid for this frame variant.
    QColor centerColor;
    QCache<uint, QRegion> cachedMasks;
    static const int MAX_CACHED_MASKS = 10;
    uint lastModified = 0;

    // Those sizes are in logical pixels
    QSizeF frameSize;
    uint cacheId;

    // measures
    qreal topHeight;
    qreal leftWidth;
    qreal rightWidth;
    qreal bottomHeight;

    // margins, are equal to the measures by default
    qreal topMargin;
    qreal leftMargin;
    qreal rightMargin;
    qreal bottomMargin;

    // measures
    qreal fixedTopHeight;
    qreal fixedLeftWidth;
    qreal fixedRightWidth;
    qreal fixedBottomHeight;

    // margins, are equal to the measures by default
    qreal fixedTopMargin;
    qreal fixedLeftMargin;
    qreal fixedRightMargin;
    qreal fixedBottomMargin;

    // margins, we only have the hqreal for insets
    qreal insetTopMargin;
    qreal insetLeftMargin;
    qreal insetRightMargin;
    qreal insetBottomMargin;

    // size of the svg where the size of the "center"
    // element is contentWidth x contentHeight
    bool noBorderPadding : 1;
    bool stretchBorders : 1;
    bool tileCenter : 1;
    bool composeOverBorder : 1;
    // Whether solidCenterColor() has run for this frame variant, and its outcome.
    bool centerSolidValid : 1;
    bool centerIsSolid : 1;

    KSvg::ImageSetPrivate *imageSet;
};

class FrameSvgPrivate
{
public:
    FrameSvgPrivate(FrameSvg *psvg)
        : q(psvg)
        , overlayPos(0, 0)
        , enabledBorders(FrameSvg::AllBorders)
        , cacheAll(false)
        , repaintBlocked(false)
    {
    }

    ~FrameSvgPrivate();

    QPixmap alphaMask();

    enum UpdateType {
        UpdateFrame,
        UpdateFrameAndMargins,
    };

    void generateBackground(const QSharedPointer<FrameData> &frame);
    void generateFrameBackground(const QSharedPointer<FrameData> &);
    SvgPrivate::CacheId cacheId(FrameData *frame, const QString &prefixToUse) const;
    void cacheFrame(const QString &prefixToSave, const QPixmap &background, const QPixmap &overlay);
    void updateSizes(FrameData *frame) const;
    void updateSizes(const QSharedPointer<FrameData> &frame) const
    {
        return updateSizes(frame.data());
    }
    void updateNeeded();
    void updateAndSignalSizes();
    QSizeF frameSize(const QSharedPointer<FrameData> &frame) const
    {
        return frameSize(frame.data());
    }
    QSizeF frameSize(FrameData *frame) const;

    // paintBorder, paintCorder and paintCenter sizes are in device pixels
    void paintBorder(QPainter &p,
                     const QSharedPointer<FrameData> &frame,
                     KSvg::FrameSvg::EnabledBorders border,
                     const QSizeF &originalSize,
                     const QRectF &output) const;
    void paintCorner(QPainter &p, const QSharedPointer<FrameData> &frame, KSvg::FrameSvg::EnabledBorders border, const QRectF &output) const;
    void paintCenter(QPainter &p, const QSharedPointer<FrameData> &frame, const QRectF &contentRect, const QSizeF &fullSize);
    // Returns true and sets color if the frame's "center" element rasterizes to a single uniform
    // color, so it can be drawn as a flat fill instead of a full-size texture.
    // Defined inline so both the FrameSvg painting code and the QML item can share one detector.
    bool solidCenterColor(const QSharedPointer<FrameData> &frame, QColor &color)
    {
        if (frame->centerSolidValid) {
            color = frame->centerColor;
            return frame->centerIsSolid;
        }

        frame->centerSolidValid = true;
        frame->centerIsSolid = false;

        // Whether the center is a single color, and which one, depends on the image, prefix, color set
        // and overrides, but never on the frame size, so the outcome is cached across sizes and only
        // recomputed when one of those changes. Frames are read from the render thread only while the
        // GUI thread is blocked for synchronization, so this shares the lock-free access of s_sharedFrames.
        size_t colorsHash = 0;
        for (const auto &[styleColor, color] : frame->colorOverrides.asKeyValueRange()) {
            colorsHash = qHashMulti(colorsHash, int(styleColor), color.rgba());
        }
        const size_t key = qHashMulti(colorsHash, frame->imagePath, frame->prefix, int(q->status()), frame->colorSet, q->Svg::d->lastModified);

        static QHash<size_t, QPair<bool, QColor>> s_solidCenters;
        const auto cached = s_solidCenters.constFind(key);
        if (cached != s_solidCenters.constEnd()) {
            frame->centerIsSolid = cached->first;
            frame->centerColor = cached->second;
            color = cached->second;
            return cached->first;
        }

        const QString centerElementId = frame->prefix % QLatin1String("center");
        // Render the element at its native size, which is small, and check whether every pixel is the
        // same. A recolored flat element (currentColor from the color scheme) resolves to one color, so
        // it can be drawn as a fill; a gradient or detailed element does not and keeps the texture path.
        const QImage image = q->image(q->elementSize(centerElementId).toSize(), centerElementId).convertToFormat(QImage::Format_ARGB32);
        if (!image.isNull()) {
            const QRgb first = image.pixel(0, 0);
            bool uniform = true;
            for (int y = 0; y < image.height() && uniform; ++y) {
                const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
                uniform = std::ranges::all_of(line, line + image.width(), [first](QRgb pixel) {
                    return pixel == first;
                });
            }
            if (uniform) {
                frame->centerIsSolid = true;
                frame->centerColor = QColor::fromRgba(first);
            }
        }

        const bool solid = frame->centerIsSolid;
        s_solidCenters.insert(key, qMakePair(solid, frame->centerColor));

        color = frame->centerColor;
        return frame->centerIsSolid;
    }

    QRectF contentGeometry(const QSharedPointer<FrameData> &frame, const QSizeF &size) const;
    void updateFrameData(uint lastModified, UpdateType updateType = UpdateFrameAndMargins);
    QSharedPointer<FrameData> lookupOrCreateMaskFrame(const QSharedPointer<FrameData> &frame, const QString &maskPrefix, const QString &maskRequestedPrefix);

    FrameSvg::LocationPrefix location = FrameSvg::Floating;
    QString prefix;
    // sometimes the prefix we requested is not available, so prefix will be empty
    // keep track of the requested one anyways, we'll try again when the theme changes
    QString requestedPrefix;

    FrameSvg *const q;

    QPointF overlayPos;

    QSharedPointer<FrameData> frame;
    QSharedPointer<FrameData> maskFrame;

    // those can differ from frame->enabledBorders if we are in a transition
    FrameSvg::EnabledBorders enabledBorders;
    // this can differ from frame->frameSize if we are in a transition
    QSizeF pendingFrameSize;

    static QHash<ImageSetPrivate *, QHash<uint, QWeakPointer<FrameData>>> s_sharedFrames;

    bool cacheAll : 1;
    bool repaintBlocked : 1;
};

}

#endif
