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
#include <QtMath>

#include <algorithm>

#include <QDebug>

#include <KSvg/ImageSet>

#include "framesvg.h"
#include "framesvg_helpers.h"
#include "svg_p.h"

namespace KSvg
{
// What one rasterization of an element says about the cheapest way to draw it at any size.
struct Uniformity {
    // Every column alike, so the element needs no more than its own width and a horizontal stretch.
    bool alongX = false;
    // Every row alike, so it needs no more than its own height and a vertical stretch.
    bool alongY = false;
    // Nothing is drawn at all, so there is nothing to render, upload or put in the scene.
    bool blank = false;
    // The one color, when it is uniform both ways.
    QRgb color = 0;

    bool uniform() const
    {
        return alongX && alongY;
    }
};

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
        , centerChecked(false)
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
        , centerChecked(false)
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
    Uniformity centerUniformity;
    // Which borders were found to repeat along the axis a frame stretches them, which of them draw
    // nothing at all, and which have been asked about. Same variant, same answer, whatever the size.
    FrameSvg::EnabledBorders stretchableBorders;
    FrameSvg::EnabledBorders blankBorders;
    FrameSvg::EnabledBorders askedBorders;
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
    // Whether the center element has been rasterized and asked about for this frame variant.
    bool centerChecked : 1;

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
    // The smallest size the uniformity check is made at, whatever the element's own size is.
    static constexpr int MinimumCheckSide = 32;

    /*
     * What one rasterization of an element says: which axes its picture repeats along, whether it draws
     * anything at all, and its color when it is one color. Every fast path here rests on this question,
     * asked once per frame variant.
     *
     * The size to ask at is the caller's, because it differs: a floor of MinimumCheckSide along an axis
     * the frame stretches, since a shape whose variation is finer than a pixel at its own size would go
     * unnoticed there and show itself once stretched, and the element's own size across an axis which
     * keeps its native measure.
     */
    Uniformity uniformity(const QString &elementId, const QSize &checkSize) const
    {
        Uniformity answer;
        const QImage image = q->image(checkSize, elementId).convertToFormat(QImage::Format_ARGB32);
        if (image.isNull()) {
            return answer;
        }

        answer.alongX = true;
        for (int y = 0; y < image.height() && answer.alongX; ++y) {
            const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            answer.alongX = std::ranges::all_of(line, line + image.width(), [line](QRgb pixel) {
                return pixel == line[0];
            });
        }

        answer.alongY = true;
        const QRgb *first = reinterpret_cast<const QRgb *>(image.constScanLine(0));
        for (int y = 1; y < image.height() && answer.alongY; ++y) {
            const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            answer.alongY = std::equal(line, line + image.width(), first);
        }

        answer.color = first[0];
        answer.blank = answer.uniform() && qAlpha(answer.color) == 0;
        return answer;
    }

    /*
     * The center element's rasterization, cached for this frame variant. Which axes it repeats along,
     * whether it draws anything, and its color if it is one color, never depend on the frame size, so
     * they are worked out once and kept.
     */
    const Uniformity &centerUniformity(const QSharedPointer<FrameData> &frame) const
    {
        if (frame->centerChecked) {
            return frame->centerUniformity;
        }
        frame->centerChecked = true;

        // The answer depends on the image, prefix, color set and overrides, but never on the frame size,
        // so it is shared by every frame of the variant. Frames are read from the render thread only
        // while the GUI thread is blocked for synchronization, so this shares the lock-free access of
        // s_sharedFrames.
        size_t colorsHash = 0;
        for (const auto &[styleColor, color] : frame->colorOverrides.asKeyValueRange()) {
            colorsHash = qHashMulti(colorsHash, int(styleColor), color.rgba());
        }
        const size_t key = qHashMulti(colorsHash, frame->imagePath, frame->prefix, int(q->status()), frame->colorSet, q->Svg::d->lastModified);

        static QHash<size_t, Uniformity> s_centers;
        const auto cached = s_centers.constFind(key);
        if (cached != s_centers.constEnd()) {
            frame->centerUniformity = *cached;
            return frame->centerUniformity;
        }

        const QString centerElementId = frame->prefix % QLatin1String("center");
        const QSizeF nativeSize = q->elementSize(centerElementId);
        // A frame stretches its center both ways, so the floor applies to both.
        const QSize checkSize(qMax(qCeil(nativeSize.width()), MinimumCheckSide), qMax(qCeil(nativeSize.height()), MinimumCheckSide));
        frame->centerUniformity = uniformity(centerElementId, checkSize);
        s_centers.insert(key, frame->centerUniformity);
        return frame->centerUniformity;
    }

    // Returns true and sets color if the frame's "center" element rasterizes to a single uniform
    // color, so it can be drawn as a flat fill instead of a full-size texture.
    // Defined inline so both the FrameSvg painting code and the QML item can share one detector.
    bool solidCenterColor(const QSharedPointer<FrameData> &frame, QColor &color) const
    {
        const Uniformity &center = centerUniformity(frame);
        color = QColor::fromRgba(center.color);
        return center.uniform();
    }

    /*
     * The axes the center repeats along when it is not one color: a center which is a gradient down its
     * height is every column alike, so it needs no more pixels across than the element has, whatever the
     * frame's width. Rendering it that narrow and stretching that axis is what rendering it at the
     * frame's width gives, pixel for pixel, since the columns are identical.
     */
    Qt::Orientations centerNativeAxes(const QSharedPointer<FrameData> &frame) const
    {
        const Uniformity &center = centerUniformity(frame);
        Qt::Orientations axes;
        if (center.alongX) {
            axes |= Qt::Horizontal;
        }
        if (center.alongY) {
            axes |= Qt::Vertical;
        }
        return axes;
    }

    /*
     * Whether the given border or the center draws nothing at all. A shadow frame is borders only, and
     * its "center" element exists but is empty, so every dialog, tooltip and panel background carrying
     * one has a part which costs a rasterization, a texture and a node to draw nothing.
     */
    bool drawsNothing(const QSharedPointer<FrameData> &frame, KSvg::FrameSvg::EnabledBorders border) const
    {
        if (border == FrameSvg::NoBorder) {
            return centerUniformity(frame).blank;
        }
        borderUniformity(frame, border);
        return frame->blankBorders & border;
    }

    /*
     * Whether the given border repeats along the axis the frame stretches it: every column alike for
     * the top and bottom, every row alike for the left and right. Such a border drawn from its native
     * size texture, stretched by the GPU, is what re-rendering it at every frame size produces, so the
     * re-render and the frame-sized texture can both go.
     *
     * The answer holds for the image, prefix and color set, not for the size, so it is cached per
     * frame variant like the centre's.
     */
    bool stretchableBorder(const QSharedPointer<FrameData> &frame, KSvg::FrameSvg::EnabledBorders border) const
    {
        borderUniformity(frame, border);
        return frame->stretchableBorders & border;
    }

private:
    // Rasterizes a border once and records both answers about it in the frame, for the whole variant.
    void borderUniformity(const QSharedPointer<FrameData> &frame, KSvg::FrameSvg::EnabledBorders border) const
    {
        if (frame->askedBorders & border) {
            return;
        }
        frame->askedBorders |= border;

        const bool horizontal = border == FrameSvg::TopBorder || border == FrameSvg::BottomBorder;
        const QString elementId = frame->prefix % FrameSvgHelpers::borderToElementId(border);
        const QSizeF nativeSize = q->elementSize(elementId);
        if (nativeSize.isEmpty()) {
            return;
        }

        // Along the stretched axis the element is asked for at a floor, for the same reason the centre
        // is. Across it the element keeps its own size, which is the thickness the frame gives it.
        const QSize checkSize = horizontal ? QSize(qMax(qCeil(nativeSize.width()), MinimumCheckSide), qCeil(nativeSize.height()))
                                           : QSize(qCeil(nativeSize.width()), qMax(qCeil(nativeSize.height()), MinimumCheckSide));
        const Uniformity side = uniformity(elementId, checkSize);
        if (horizontal ? side.alongX : side.alongY) {
            frame->stretchableBorders |= border;
        }
        if (side.blank) {
            frame->blankBorders |= border;
        }
    }

public:
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
