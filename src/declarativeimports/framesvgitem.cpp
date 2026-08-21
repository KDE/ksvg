/*
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "framesvgitem.h"

#include "imagetexturescache.h"
#include "managedtexturenode.h"

#include <QQuickWindow>
#include <QSGGeometry>
#include <QSGTexture>

#include <KColorUtils>
#include <QDebug>
#include <QPainter>

#include <ksvg/private/framesvg_helpers.h>
#include <ksvg/private/framesvg_p.h>

#include <cmath> //floor()

#include <Kirigami/Platform/PlatformTheme>
#include <debug_p.h>

namespace KSvg
{

class FrameNode : public QSGNode
{
public:
    FrameNode(const QString &prefix, FrameSvg *svg)
        : QSGNode()
        , leftWidth(0)
        , rightWidth(0)
        , topHeight(0)
        , bottomHeight(0)
    {
        if (svg->enabledBorders() & FrameSvg::LeftBorder) {
            leftWidth = svg->elementSize(prefix % QLatin1String("left")).width();
        }
        if (svg->enabledBorders() & FrameSvg::RightBorder) {
            rightWidth = svg->elementSize(prefix % QLatin1String("right")).width();
        }
        if (svg->enabledBorders() & FrameSvg::TopBorder) {
            topHeight = svg->elementSize(prefix % QLatin1String("top")).height();
        }
        if (svg->enabledBorders() & FrameSvg::BottomBorder) {
            bottomHeight = svg->elementSize(prefix % QLatin1String("bottom")).height();
        }
    }

    QRect contentsRect(const QSize &size) const
    {
        const QSize contentSize(size.width() - leftWidth - rightWidth, size.height() - topHeight - bottomHeight);

        return QRect(QPoint(leftWidth, topHeight), contentSize);
    }

private:
    int leftWidth;
    int rightWidth;
    int topHeight;
    int bottomHeight;
};

class FrameItemNode : public ManagedTextureNode
{
public:
    enum FitMode {
        // render SVG at native resolution then stretch it in openGL
        FastStretch,
        // on resize re-render the part of the frame from the SVG
        Stretch,
        Tile,
    };

    FrameItemNode(FrameSvgItem *frameSvg, FrameSvg::EnabledBorders borders, FitMode fitMode, QSGNode *parent, Qt::Orientations nativeAxes = {})
        : ManagedTextureNode()
        , m_frameSvg(frameSvg)
        , m_border(borders)
        , m_fitMode(fitMode)
        , m_nativeAxes(nativeAxes)
    {
        parent->appendChildNode(this);

        if (m_fitMode == Tile) {
            if (m_border == FrameSvg::TopBorder || m_border == FrameSvg::BottomBorder || m_border == FrameSvg::NoBorder) {
                static_cast<QSGTextureMaterial *>(material())->setHorizontalWrapMode(QSGTexture::Repeat);
                static_cast<QSGOpaqueTextureMaterial *>(opaqueMaterial())->setHorizontalWrapMode(QSGTexture::Repeat);
            }
            if (m_border == FrameSvg::LeftBorder || m_border == FrameSvg::RightBorder || m_border == FrameSvg::NoBorder) {
                static_cast<QSGTextureMaterial *>(material())->setVerticalWrapMode(QSGTexture::Repeat);
                static_cast<QSGOpaqueTextureMaterial *>(opaqueMaterial())->setVerticalWrapMode(QSGTexture::Repeat);
            }
        }

        if (m_fitMode == Tile || m_fitMode == FastStretch) {
            QString elementId = m_frameSvg->frameSvg()->actualPrefix() + FrameSvgHelpers::borderToElementId(m_border);
            m_elementNativeSize = m_frameSvg->frameSvg()->elementSize(elementId).toSize();

            if (m_elementNativeSize.isEmpty()) {
                // if the default element is empty, we can avoid the slower tiling path
                // this also avoids a divide by 0 error
                m_fitMode = FastStretch;
            }

            QSize sourceSize = m_elementNativeSize;
            if (m_fitMode == FastStretch) {
                // The node takes its thickness from elementSize() truncated, so the texture is asked for at
                // the same whole number. Rounding up instead would leave the GPU resampling the axis the
                // artwork varies across: Breeze's menubaritem borders are 7.5 thick, Oxygen's scrollbar
                // background 4.989.
                const QSizeF exact = m_frameSvg->frameSvg()->elementSize(elementId);
                sourceSize = QSize(qMax(1, int(exact.width())), qMax(1, int(exact.height())));

                // A part which repeats along the axis it is stretched on carries its whole picture in one
                // row or column of itself, whatever size the frame takes, so that is all the texture has
                // to hold. The thickness is kept, since that is where the shading is. Corners arrive here
                // as a pair of borders and are never stretched, so they keep their own size.
                if (m_border == FrameSvg::TopBorder || m_border == FrameSvg::BottomBorder) {
                    sourceSize.setWidth(1);
                } else if (m_border == FrameSvg::LeftBorder || m_border == FrameSvg::RightBorder) {
                    sourceSize.setHeight(1);
                }
            }

            updateTexture(sourceSize, elementId);
        }
    }

    void updateTexture(const QSize &size, const QString &elementId)
    {
        QQuickWindow::CreateTextureOptions options;
        if (m_fitMode != Tile) {
            options = QQuickWindow::TextureCanUseAtlas;
        }
        setTexture(ImageTexturesCache::instance()->loadTexture(m_frameSvg->window(), m_frameSvg->frameSvg()->image(size, elementId), options));
    }

    void reposition(const QRect &frameGeometry, QSize &fullSize)
    {
        QRectF nodeRect = FrameSvgHelpers::sectionRect(m_border, frameGeometry, fullSize);

        // ensure we're not passing a weird rectangle to updateTexturedRectGeometry
        if (!nodeRect.isValid() || nodeRect.isEmpty()) {
            nodeRect = QRect();
        }

        // the position of the relevant texture within this texture ID.
        // for atlas' this will only be a small part of the texture
        QRectF textureRect;

        if (m_fitMode == Tile) {
            textureRect = QRectF(0, 0, 1, 1); // we can never be in an atlas for tiled images.

            // if tiling horizontally
            if (m_border == FrameSvg::TopBorder || m_border == FrameSvg::BottomBorder || m_border == FrameSvg::NoBorder) {
                // cmp. CSS3's border-image-repeat: "repeat", though with first tile not centered, but aligned to left
                textureRect.setWidth((qreal)nodeRect.width() / m_elementNativeSize.width());
            }
            // if tiling vertically
            if (m_border == FrameSvg::LeftBorder || m_border == FrameSvg::RightBorder || m_border == FrameSvg::NoBorder) {
                // cmp. CSS3's border-image-repeat: "repeat", though with first tile not centered, but aligned to top
                textureRect.setHeight((qreal)nodeRect.height() / m_elementNativeSize.height());
            }
        } else if (m_fitMode == Stretch) {
            QString prefix = m_frameSvg->frameSvg()->actualPrefix();

            QString elementId = prefix + FrameSvgHelpers::borderToElementId(m_border);

            // Re-render the SVG at the new size, except along an axis the theme says the picture repeats
            // along: one column of a picture whose columns are alike is the whole of it, so a single pixel
            // there is enough and the node stretches it back out.
            QSize renderSize = nodeRect.size().toSize();
            if (m_nativeAxes & Qt::Horizontal) {
                renderSize.setWidth(1);
            }
            if (m_nativeAxes & Qt::Vertical) {
                renderSize.setHeight(1);
            }
            updateTexture(renderSize, elementId);
            textureRect = texture()->normalizedTextureSubRect();
        } else if (texture()) { // for fast stretch.
            textureRect = texture()->normalizedTextureSubRect();
        }

        QSGGeometry::updateTexturedRectGeometry(geometry(), nodeRect, textureRect);
        markDirty(QSGNode::DirtyGeometry);
    }

private:
    FrameSvgItem *m_frameSvg;
    FrameSvg::EnabledBorders m_border;
    QSize m_elementNativeSize;
    FitMode m_fitMode;
    Qt::Orientations m_nativeAxes;
};

FrameSvgItemMargins::FrameSvgItemMargins(KSvg::FrameSvg *frameSvg, QObject *parent)
    : QObject(parent)
    , m_frameSvg(frameSvg)
    , m_fixed(false)
    , m_inset(false)
{
    // qDebug() << "margins at: " << left() << top() << right() << bottom();
}

qreal FrameSvgItemMargins::left() const
{
    if (m_fixed) {
        return m_frameSvg->fixedMarginSize(FrameSvg::LeftMargin);
    } else if (m_inset) {
        return m_frameSvg->insetSize(FrameSvg::LeftMargin);
    } else {
        return m_frameSvg->marginSize(FrameSvg::LeftMargin);
    }
}

qreal FrameSvgItemMargins::top() const
{
    if (m_fixed) {
        return m_frameSvg->fixedMarginSize(FrameSvg::TopMargin);
    } else if (m_inset) {
        return m_frameSvg->insetSize(FrameSvg::TopMargin);
    } else {
        return m_frameSvg->marginSize(FrameSvg::TopMargin);
    }
}

qreal FrameSvgItemMargins::right() const
{
    if (m_fixed) {
        return m_frameSvg->fixedMarginSize(FrameSvg::RightMargin);
    } else if (m_inset) {
        return m_frameSvg->insetSize(FrameSvg::RightMargin);
    } else {
        return m_frameSvg->marginSize(FrameSvg::RightMargin);
    }
}

qreal FrameSvgItemMargins::bottom() const
{
    if (m_fixed) {
        return m_frameSvg->fixedMarginSize(FrameSvg::BottomMargin);
    } else if (m_inset) {
        return m_frameSvg->insetSize(FrameSvg::BottomMargin);
    } else {
        return m_frameSvg->marginSize(FrameSvg::BottomMargin);
    }
}

qreal FrameSvgItemMargins::horizontal() const
{
    return left() + right();
}

qreal FrameSvgItemMargins::vertical() const
{
    return top() + bottom();
}

QList<qreal> FrameSvgItemMargins::margins() const
{
    qreal left;
    qreal top;
    qreal right;
    qreal bottom;
    m_frameSvg->getMargins(left, top, right, bottom);
    return {left, top, right, bottom};
}

void FrameSvgItemMargins::update()
{
    Q_EMIT marginsChanged();
}

void FrameSvgItemMargins::setFixed(bool fixed)
{
    if (fixed == m_fixed) {
        return;
    }

    m_fixed = fixed;
    Q_EMIT marginsChanged();
}

bool FrameSvgItemMargins::isFixed() const
{
    return m_fixed;
}

void FrameSvgItemMargins::setInset(bool inset)
{
    if (inset == m_inset) {
        return;
    }

    m_inset = inset;
    Q_EMIT marginsChanged();
}

bool FrameSvgItemMargins::isInset() const
{
    return m_inset;
}

FrameSvgItem::FrameSvgItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_margins(nullptr)
    , m_fixedMargins(nullptr)
    , m_insetMargins(nullptr)
    , m_textureChanged(false)
    , m_sizeChanged(false)
    , m_fastPath(true)
{
    m_frameSvg = new KSvg::FrameSvg(this);

    setFlag(QQuickItem::ItemHasContents, true);
    setFlag(ItemHasContents, true);
    connect(m_frameSvg, &FrameSvg::repaintNeeded, this, &FrameSvgItem::doUpdate);
    connect(m_frameSvg, &Svg::fromCurrentImageSetChanged, this, &FrameSvgItem::fromCurrentImageSetChanged);
    connect(m_frameSvg, &Svg::statusChanged, this, &FrameSvgItem::statusChanged);
}

FrameSvgItem::~FrameSvgItem()
{
}

class CheckMarginsChange
{
public:
    CheckMarginsChange(QList<qreal> &oldMargins, FrameSvgItemMargins *marginsObject)
        : m_oldMargins(oldMargins)
        , m_marginsObject(marginsObject)
    {
    }

    ~CheckMarginsChange()
    {
        const QList<qreal> oldMarginsBefore = m_oldMargins;
        m_oldMargins = m_marginsObject ? m_marginsObject->margins() : QList<qreal>();

        if (m_marginsObject && oldMarginsBefore != m_oldMargins) {
            m_marginsObject->update();
        }
    }

private:
    QList<qreal> &m_oldMargins;
    FrameSvgItemMargins *const m_marginsObject;
};

void FrameSvgItem::setImagePath(const QString &path)
{
    if (m_frameSvg->imagePath() == path) {
        return;
    }

    CheckMarginsChange checkMargins(m_oldMargins, m_margins);
    CheckMarginsChange checkFixedMargins(m_oldFixedMargins, m_fixedMargins);
    CheckMarginsChange checkInsetMargins(m_oldInsetMargins, m_insetMargins);

    updateDevicePixelRatio();
    m_frameSvg->setImagePath(path);

    if (implicitWidth() <= 0) {
        setImplicitWidth(m_frameSvg->marginSize(KSvg::FrameSvg::LeftMargin) + m_frameSvg->marginSize(KSvg::FrameSvg::RightMargin));
    }

    if (implicitHeight() <= 0) {
        setImplicitHeight(m_frameSvg->marginSize(KSvg::FrameSvg::TopMargin) + m_frameSvg->marginSize(KSvg::FrameSvg::BottomMargin));
    }

    Q_EMIT imagePathChanged();

    if (isComponentComplete()) {
        applyPrefixes();

        m_frameSvg->resizeFrame(size());
        m_textureChanged = true;
        update();
    }
}

QString FrameSvgItem::imagePath() const
{
    return m_frameSvg->imagePath();
}

void FrameSvgItem::setPrefix(const QVariant &prefixes)
{
    QStringList prefixList;
    // is this a simple string?
    if (prefixes.canConvert<QString>()) {
        prefixList << prefixes.toString();
    } else if (prefixes.canConvert<QStringList>()) {
        prefixList = prefixes.toStringList();
    }

    if (m_prefixes == prefixList) {
        return;
    }

    CheckMarginsChange checkMargins(m_oldMargins, m_margins);
    CheckMarginsChange checkFixedMargins(m_oldFixedMargins, m_fixedMargins);
    CheckMarginsChange checkInsetMargins(m_oldInsetMargins, m_insetMargins);

    m_prefixes = prefixList;
    applyPrefixes();

    if (implicitWidth() <= 0) {
        setImplicitWidth(m_frameSvg->marginSize(KSvg::FrameSvg::LeftMargin) + m_frameSvg->marginSize(KSvg::FrameSvg::RightMargin));
    }

    if (implicitHeight() <= 0) {
        setImplicitHeight(m_frameSvg->marginSize(KSvg::FrameSvg::TopMargin) + m_frameSvg->marginSize(KSvg::FrameSvg::BottomMargin));
    }

    Q_EMIT prefixChanged();

    if (isComponentComplete()) {
        m_frameSvg->resizeFrame(QSizeF(width(), height()));
        m_textureChanged = true;
        update();
    }
}

QVariant FrameSvgItem::prefix() const
{
    return m_prefixes;
}

QString FrameSvgItem::usedPrefix() const
{
    return m_frameSvg->prefix();
}

FrameSvgItemMargins *FrameSvgItem::margins()
{
    if (!m_margins) {
        m_margins = new FrameSvgItemMargins(m_frameSvg, this);
    }
    return m_margins;
}

FrameSvgItemMargins *FrameSvgItem::fixedMargins()
{
    if (!m_fixedMargins) {
        m_fixedMargins = new FrameSvgItemMargins(m_frameSvg, this);
        m_fixedMargins->setFixed(true);
    }
    return m_fixedMargins;
}

FrameSvgItemMargins *FrameSvgItem::inset()
{
    if (!m_insetMargins) {
        m_insetMargins = new FrameSvgItemMargins(m_frameSvg, this);
        m_insetMargins->setInset(true);
    }
    return m_insetMargins;
}

bool FrameSvgItem::fromCurrentImageSet() const
{
    return m_frameSvg->fromCurrentImageSet();
}

void FrameSvgItem::setStatus(KSvg::Svg::Status status)
{
    m_frameSvg->setStatus(status);
}

KSvg::Svg::Status FrameSvgItem::status() const
{
    return m_frameSvg->status();
}

void FrameSvgItem::setEnabledBorders(const KSvg::FrameSvg::EnabledBorders borders)
{
    if (m_frameSvg->enabledBorders() == borders) {
        return;
    }

    CheckMarginsChange checkMargins(m_oldMargins, m_margins);

    m_frameSvg->setEnabledBorders(borders);
    Q_EMIT enabledBordersChanged();
    m_textureChanged = true;
    update();
}

KSvg::FrameSvg::EnabledBorders FrameSvgItem::enabledBorders() const
{
    return m_frameSvg->enabledBorders();
}

void FrameSvgItem::setColorSet(KSvg::Svg::ColorSet colorSet)
{
    if (m_frameSvg->colorSet() == colorSet) {
        return;
    }

    m_frameSvg->setColorSet(colorSet);
    m_textureChanged = true;

    update();
}

KSvg::Svg::ColorSet FrameSvgItem::colorSet() const
{
    return m_frameSvg->colorSet();
}

bool FrameSvgItem::hasElementPrefix(const QString &prefix) const
{
    return m_frameSvg->hasElementPrefix(prefix);
}

bool FrameSvgItem::hasElement(const QString &elementName) const
{
    return m_frameSvg->hasElement(elementName);
}

QRegion FrameSvgItem::mask() const
{
    return m_frameSvg->mask();
}

int FrameSvgItem::minimumDrawingHeight() const
{
    return m_frameSvg->minimumDrawingHeight();
}

int FrameSvgItem::minimumDrawingWidth() const
{
    return m_frameSvg->minimumDrawingWidth();
}

void FrameSvgItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    const bool isComponentComplete = this->isComponentComplete();
    if (isComponentComplete) {
        m_frameSvg->resizeFrame(newGeometry.size());
        m_sizeChanged = true;
    }

    QQuickItem::geometryChange(newGeometry, oldGeometry);

    // the above only triggers updatePaintNode, so we have to inform subscribers
    // about the potential change of the mask explicitly here
    if (isComponentComplete) {
        Q_EMIT maskChanged();
    }
}

void FrameSvgItem::doUpdate()
{
    if (m_frameSvg->isRepaintBlocked()) {
        return;
    }

    CheckMarginsChange checkMargins(m_oldMargins, m_margins);
    CheckMarginsChange checkFixedMargins(m_oldFixedMargins, m_fixedMargins);
    CheckMarginsChange checkInsetMargins(m_oldInsetMargins, m_insetMargins);

    // if the theme changed, the available prefix may have changed as well
    applyPrefixes();

    if (implicitWidth() <= 0) {
        setImplicitWidth(m_frameSvg->marginSize(KSvg::FrameSvg::LeftMargin) + m_frameSvg->marginSize(KSvg::FrameSvg::RightMargin));
    }

    if (implicitHeight() <= 0) {
        setImplicitHeight(m_frameSvg->marginSize(KSvg::FrameSvg::TopMargin) + m_frameSvg->marginSize(KSvg::FrameSvg::BottomMargin));
    }

    QString prefix = m_frameSvg->actualPrefix();
    bool hasOverlay = (!prefix.startsWith(QLatin1String("mask-")) //
                       && m_frameSvg->hasElement(prefix % QLatin1String("overlay")));
    bool hasComposeOverBorder = m_frameSvg->hasElement(prefix % QLatin1String("hint-compose-over-border"))
        && m_frameSvg->hasElement(QLatin1String("mask-") % prefix % QLatin1String("center"));
    m_fastPath = !hasOverlay && !hasComposeOverBorder;

    // Software rendering (at time of writing Qt5.10) doesn't seem to like our
    // tiling/stretching in the 9-tiles.
    // Also when using QPainter it's arguably faster to create and cache pixmaps
    // of the whole frame, which is what the slow path does
    if (QQuickWindow::sceneGraphBackend() == QLatin1String("software")) {
        m_fastPath = false;
    }
    m_textureChanged = true;

    update();

    Q_EMIT maskChanged();
    Q_EMIT repaintNeeded();
}

KSvg::FrameSvg *FrameSvgItem::frameSvg() const
{
    return m_frameSvg;
}

QSGNode *FrameSvgItem::updatePaintNode(QSGNode *oldNode, QQuickItem::UpdatePaintNodeData *)
{
    if (!window() || !m_frameSvg //
        || (!m_frameSvg->hasElementPrefix(m_frameSvg->actualPrefix()) //
            && !m_frameSvg->hasElementPrefix(m_frameSvg->prefix()))) {
        delete oldNode;
        return nullptr;
    }

    const QSGTexture::Filtering filtering = smooth() ? QSGTexture::Linear : QSGTexture::Nearest;

    if (m_fastPath) {
        if (m_textureChanged) {
            delete oldNode;
            oldNode = nullptr;
        }

        if (!oldNode) {
            QString prefix = m_frameSvg->actualPrefix();
            oldNode = new FrameNode(prefix, m_frameSvg);

            bool tileCenter = (m_frameSvg->hasElement(QStringLiteral("hint-tile-center")) //
                               || m_frameSvg->hasElement(prefix % QLatin1String("hint-tile-center")));
            bool stretchBorders = (m_frameSvg->hasElement(QStringLiteral("hint-stretch-borders")) //
                                   || m_frameSvg->hasElement(prefix % QLatin1String("hint-stretch-borders")));
            FrameItemNode::FitMode borderFitMode = stretchBorders ? FrameItemNode::Stretch : FrameItemNode::Tile;
            FrameItemNode::FitMode centerFitMode = tileCenter ? FrameItemNode::Tile : FrameItemNode::Stretch;

            // The same shape as the hints above: unprefixed for a whole image, prefixed for one frame of it.
            auto hinted = [this, &prefix](const QString &hint) {
                return m_frameSvg->hasElement(hint) || m_frameSvg->hasElement(prefix % hint);
            };
            const bool solidCenter = hinted(QStringLiteral("hint-solid-color"));

            // A border is only ever stretched along its length. Where the theme says a border varies just
            // across its thickness, the picture at any length is the one the GPU can make from the element's
            // own texture, so the re-render at every size goes. Where it does not say so, a stretched border
            // keeps being rendered at size: that is the only correct reading when the artwork changes along
            // the length.
            //
            // Whether it holds is a question per side, not per frame: of Oxygen's frames which ask for
            // stretched borders, four have all four sides uniform and fifty have some. hint-uniform-borders
            // answers for all four at once, hint-uniform-top-border and its three siblings answer for one,
            // and either is enough for the side it covers.
            const bool uniformBorders = hinted(QStringLiteral("hint-uniform-borders"));
            auto borderFit = [&hinted, borderFitMode, uniformBorders](FrameSvg::EnabledBorders border) {
                if (borderFitMode != FrameItemNode::Stretch) {
                    return borderFitMode;
                }
                if (uniformBorders || hinted(QLatin1String("hint-uniform-") % FrameSvgHelpers::borderToElementId(border) % QLatin1String("-border"))) {
                    return FrameItemNode::FastStretch;
                }
                return borderFitMode;
            };
            Qt::Orientations centerNativeAxes;
            if (hinted(QLatin1String("hint-stretch-center-horizontally"))) {
                centerNativeAxes |= Qt::Horizontal;
            }
            if (hinted(QLatin1String("hint-stretch-center-vertically"))) {
                centerNativeAxes |= Qt::Vertical;
            }

            if (solidCenter) {
                // One colour repeats along both axes, so one pixel of it is the whole of it and the node
                // stretches that across the content, whatever size the frame takes.
                centerNativeAxes = Qt::Horizontal | Qt::Vertical;

                // One colour which happens to be no colour: a shadow frame is borders only and its center
                // element is empty. Nothing is drawn for it at all.
                const QImage sample = m_frameSvg->image(QSize(3, 3), prefix % QLatin1String("center"));
                const bool drawsSomething = !sample.isNull() && sample.pixelColor(1, 1).alpha() > 0;
                if (drawsSomething) {
                    new FrameItemNode(this, FrameSvg::NoBorder, FrameItemNode::Stretch, oldNode, centerNativeAxes);
                }
            } else {
                new FrameItemNode(this, FrameSvg::NoBorder, centerFitMode, oldNode, centerNativeAxes);
            }
            if (enabledBorders() & (FrameSvg::TopBorder | FrameSvg::LeftBorder)) {
                new FrameItemNode(this, FrameSvg::TopBorder | FrameSvg::LeftBorder, FrameItemNode::FastStretch, oldNode);
            }
            if (enabledBorders() & (FrameSvg::TopBorder | FrameSvg::RightBorder)) {
                new FrameItemNode(this, FrameSvg::TopBorder | FrameSvg::RightBorder, FrameItemNode::FastStretch, oldNode);
            }
            if (enabledBorders() & FrameSvg::TopBorder) {
                new FrameItemNode(this, FrameSvg::TopBorder, borderFit(FrameSvg::TopBorder), oldNode);
            }
            if (enabledBorders() & FrameSvg::BottomBorder) {
                new FrameItemNode(this, FrameSvg::BottomBorder, borderFit(FrameSvg::BottomBorder), oldNode);
            }
            if (enabledBorders() & (FrameSvg::BottomBorder | FrameSvg::LeftBorder)) {
                new FrameItemNode(this, FrameSvg::BottomBorder | FrameSvg::LeftBorder, FrameItemNode::FastStretch, oldNode);
            }
            if (enabledBorders() & (FrameSvg::BottomBorder | FrameSvg::RightBorder)) {
                new FrameItemNode(this, FrameSvg::BottomBorder | FrameSvg::RightBorder, FrameItemNode::FastStretch, oldNode);
            }
            if (enabledBorders() & FrameSvg::LeftBorder) {
                new FrameItemNode(this, FrameSvg::LeftBorder, borderFit(FrameSvg::LeftBorder), oldNode);
            }
            if (enabledBorders() & FrameSvg::RightBorder) {
                new FrameItemNode(this, FrameSvg::RightBorder, borderFit(FrameSvg::RightBorder), oldNode);
            }

            m_sizeChanged = true;
            m_textureChanged = false;
        }

        QSGNode *node = oldNode->firstChild();
        while (node) {
            static_cast<FrameItemNode *>(node)->setFiltering(filtering);
            node = node->nextSibling();
        }

        if (m_sizeChanged) {
            FrameNode *frameNode = static_cast<FrameNode *>(oldNode);
            QSize frameSize(width(), height());
            QRect geometry = frameNode->contentsRect(frameSize);
            QSGNode *node = oldNode->firstChild();
            while (node) {
                static_cast<FrameItemNode *>(node)->reposition(geometry, frameSize);
                node = node->nextSibling();
            }

            m_sizeChanged = false;
        }
    } else {
        ManagedTextureNode *textureNode = dynamic_cast<ManagedTextureNode *>(oldNode);
        if (!textureNode) {
            delete oldNode;
            textureNode = new ManagedTextureNode;
            m_textureChanged = true; // force updating the texture on our newly created node
            oldNode = textureNode;
        }
        textureNode->setFiltering(filtering);

        if ((m_textureChanged || m_sizeChanged) || textureNode->texture()->textureSize() != m_frameSvg->size()) {
            QImage image = m_frameSvg->framePixmap().toImage();
            textureNode->setTexture(ImageTexturesCache::instance()->loadTexture(window(), image));
            textureNode->setRect(0, 0, width(), height());

            m_textureChanged = false;
            m_sizeChanged = false;
        }
    }

    return oldNode;
}

void FrameSvgItem::classBegin()
{
    QQuickItem::classBegin();
    m_frameSvg->setRepaintBlocked(true);
}

void FrameSvgItem::componentComplete()
{
    m_kirigamiTheme = qobject_cast<Kirigami::Platform::PlatformTheme *>(qmlAttachedPropertiesObject<Kirigami::Platform::PlatformTheme>(this, true));
    if (!m_kirigamiTheme) {
        qCWarning(LOG_KSVGQML) << "no theme!" << qmlAttachedPropertiesObject<Kirigami::Platform::PlatformTheme>(this, true) << this;
        return;
    }

    auto checkApplyTheme = [this]() {
        if (!m_frameSvg->imageSet()->filePath(QStringLiteral("colors")).isEmpty()) {
            m_frameSvg->clearCache();
            m_frameSvg->clearColorOverrides();
        }
    };
    auto applyTheme = [this]() {
        if (!m_frameSvg->imageSet()->filePath(QStringLiteral("colors")).isEmpty()) {
            m_frameSvg->clearCache();
            m_frameSvg->clearColorOverrides();

            return;
        }
        m_frameSvg->setColors(
            {{Svg::Text, m_kirigamiTheme->textColor()},
             {Svg::Background, m_kirigamiTheme->backgroundColor()},
             {Svg::Highlight, m_kirigamiTheme->highlightColor()},
             {Svg::HighlightedText, m_kirigamiTheme->highlightedTextColor()},
             {Svg::PositiveText, m_kirigamiTheme->positiveTextColor()},
             {Svg::NeutralText, m_kirigamiTheme->neutralTextColor()},
             {Svg::NegativeText, m_kirigamiTheme->negativeTextColor()},
             {Svg::Frame, KColorUtils::mix(m_kirigamiTheme->backgroundColor(), m_kirigamiTheme->textColor(), KColorScheme::frameContrast())}});
    };

    applyTheme();
    connect(m_kirigamiTheme, &Kirigami::Platform::PlatformTheme::frameContrastChanged, this, applyTheme);
    connect(m_kirigamiTheme, &Kirigami::Platform::PlatformTheme::colorsChanged, this, applyTheme);
    connect(m_frameSvg->imageSet(), &ImageSet::imageSetChanged, this, checkApplyTheme);
    connect(m_frameSvg, &Svg::imageSetChanged, this, checkApplyTheme);

    CheckMarginsChange checkMargins(m_oldMargins, m_margins);
    CheckMarginsChange checkFixedMargins(m_oldFixedMargins, m_fixedMargins);
    CheckMarginsChange checkInsetMargins(m_oldInsetMargins, m_insetMargins);

    QQuickItem::componentComplete();
    m_frameSvg->resizeFrame(size());
    m_frameSvg->setRepaintBlocked(false);
    m_textureChanged = true;
}

void FrameSvgItem::updateDevicePixelRatio()
{
    const auto newDevicePixelRatio = std::max<qreal>(1.0, (window() ? window()->devicePixelRatio() : qApp->devicePixelRatio()));
    if (newDevicePixelRatio != m_frameSvg->devicePixelRatio()) {
        m_frameSvg->setDevicePixelRatio(newDevicePixelRatio);
        m_textureChanged = true;
    }
}

void FrameSvgItem::applyPrefixes()
{
    if (m_frameSvg->imagePath().isEmpty()) {
        return;
    }

    const QString oldPrefix = m_frameSvg->prefix();

    if (m_prefixes.isEmpty()) {
        m_frameSvg->setElementPrefix(QString());
        if (oldPrefix != m_frameSvg->prefix()) {
            Q_EMIT usedPrefixChanged();
        }
        return;
    }

    bool found = false;
    for (const QString &prefix : std::as_const(m_prefixes)) {
        if (m_frameSvg->hasElementPrefix(prefix)) {
            m_frameSvg->setElementPrefix(prefix);
            found = true;
            break;
        }
    }
    if (!found) {
        // this setElementPrefix is done to keep the same behavior as before, when it was a simple string
        m_frameSvg->setElementPrefix(m_prefixes.constLast());
    }
    if (oldPrefix != m_frameSvg->prefix()) {
        Q_EMIT usedPrefixChanged();
    }
}

void FrameSvgItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    if (change == ItemSceneChange && value.window) {
        updateDevicePixelRatio();
    } else if (change == QQuickItem::ItemDevicePixelRatioHasChanged) {
        updateDevicePixelRatio();
    }

    QQuickItem::itemChange(change, value);
}

} // KSvg namespace

#include "moc_framesvgitem.cpp"
