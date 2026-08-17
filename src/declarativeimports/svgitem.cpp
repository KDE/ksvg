/*
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "svgitem.h"

#include <QDebug>
#include <QQuickWindow>
#include <QtMath>

#include <QRectF>
#include <QSGTexture>
#include <algorithm>

#include "ksvg/svg.h"

#include "imagetexturescache.h"
#include "managedtexturenode.h"

#include <KColorScheme>
#include <KColorUtils>
#include <Kirigami/Platform/PlatformTheme>
#include <debug_p.h>

namespace KSvg
{
namespace
{
// Whether an element's rasterisation repeats along an axis: every column alike, so it can be drawn from
// one column and stretched across, or every row alike, so it can be drawn from one row and stretched down.
// An element which repeats needs no more pixels than its own size, however large the item showing it is.
struct RepeatsAlong {
    bool horizontally = false;
    bool vertically = false;
};

// The size the element is asked for when the question is put. Its own size is not enough: a shape whose
// variation is finer than a pixel there would go unnoticed and show itself once stretched.
constexpr int MinimumCheckSide = 32;

RepeatsAlong repeatsAlong(KSvg::Svg *svg, const QString &elementId, const QSizeF &nativeSize)
{
    // The answer holds for the image, the element, the colour set and the theme, not for the item's size,
    // so it is worked out once and kept.
    static QHash<size_t, RepeatsAlong> s_answers;
    const size_t key = qHashMulti(0, svg->imagePath(), elementId, int(svg->colorSet()), int(svg->status()), svg->devicePixelRatio());
    const auto known = s_answers.constFind(key);
    if (known != s_answers.constEnd()) {
        return *known;
    }

    RepeatsAlong answer;
    const QSize checkSize(qMax(qCeil(nativeSize.width()), MinimumCheckSide), qMax(qCeil(nativeSize.height()), MinimumCheckSide));
    const QImage image = svg->image(checkSize, elementId).convertToFormat(QImage::Format_ARGB32);
    if (!image.isNull()) {
        answer.horizontally = true;
        for (int y = 0; y < image.height() && answer.horizontally; ++y) {
            const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            answer.horizontally = std::all_of(line, line + image.width(), [line](QRgb pixel) {
                return pixel == line[0];
            });
        }

        answer.vertically = true;
        const QRgb *first = reinterpret_cast<const QRgb *>(image.constScanLine(0));
        for (int y = 1; y < image.height() && answer.vertically; ++y) {
            const QRgb *line = reinterpret_cast<const QRgb *>(image.constScanLine(y));
            answer.vertically = std::equal(line, line + image.width(), first);
        }
    }

    s_answers.insert(key, answer);
    return answer;
}
}

SvgItem::SvgItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_textureChanged(false)
{
    m_svg = new KSvg::Svg(this);
    setFlag(QQuickItem::ItemHasContents, true);

    connect(m_svg, &Svg::repaintNeeded, this, &SvgItem::updateNeeded);
    connect(m_svg, &Svg::repaintNeeded, this, &SvgItem::naturalSizeChanged);
    connect(m_svg, &Svg::sizeChanged, this, &SvgItem::naturalSizeChanged);
    connect(m_svg, &Svg::repaintNeeded, this, &SvgItem::elementRectChanged);
    connect(m_svg, &Svg::sizeChanged, this, &SvgItem::elementRectChanged);
}

SvgItem::~SvgItem()
{
    // Make sure to not call anything on m_svg when this is shutting down
    // Kirigami::PlatformTheme will lose its window at that point so will
    // emit colorschanged, which we shouldn't react to during destructor
    disconnect(m_kirigamiTheme, nullptr, this, nullptr);
}

void SvgItem::componentComplete()
{
    m_kirigamiTheme = qobject_cast<Kirigami::Platform::PlatformTheme *>(qmlAttachedPropertiesObject<Kirigami::Platform::PlatformTheme>(this, true));
    if (!m_kirigamiTheme) {
        qCWarning(LOG_KSVGQML) << "No theme!" << qmlAttachedPropertiesObject<Kirigami::Platform::PlatformTheme>(this, true) << this;
        return;
    }

    auto checkApplyTheme = [this]() {
        if (!m_svg->imageSet()->filePath(QStringLiteral("colors")).isEmpty()) {
            m_svg->clearColorOverrides();
        }
    };
    auto applyTheme = [this]() {
        if (!m_svg) {
            return;
        }
        if (!m_svg->imageSet()->filePath(QStringLiteral("colors")).isEmpty()) {
            m_svg->clearColorOverrides();
            return;
        }
        m_svg->setColors({{Svg::Text, m_kirigamiTheme->textColor()},
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
    connect(m_svg->imageSet(), &ImageSet::imageSetChanged, this, checkApplyTheme);
    connect(m_svg, &Svg::imageSetChanged, this, checkApplyTheme);

    QQuickItem::componentComplete();
}

void SvgItem::setImagePath(const QString &path)
{
    if (!m_svg || m_svg->imagePath() == path) {
        return;
    }

    updateDevicePixelRatio();
    m_svg->setImagePath(path);

    Q_EMIT imagePathChanged();

    if (isComponentComplete()) {
        update();
    }
}

QString SvgItem::imagePath() const
{
    return m_svg->imagePath();
}

void SvgItem::setElementId(const QString &elementID)
{
    if (elementID == m_elementID) {
        return;
    }

    if (implicitWidth() <= 0) {
        setImplicitWidth(naturalSize().width());
    }
    if (implicitHeight() <= 0) {
        setImplicitHeight(naturalSize().height());
    }

    m_elementID = elementID;
    Q_EMIT elementIdChanged();
    Q_EMIT naturalSizeChanged();
    Q_EMIT elementRectChanged();

    scheduleImageUpdate();
}

QString SvgItem::elementId() const
{
    return m_elementID;
}

void SvgItem::setSvg(KSvg::Svg *svg)
{
    if (m_svg) {
        disconnect(m_svg.data(), nullptr, this, nullptr);
    }
    m_svg = svg;
    updateDevicePixelRatio();

    if (svg) {
        connect(svg, &Svg::repaintNeeded, this, &SvgItem::updateNeeded);
        connect(svg, &Svg::repaintNeeded, this, &SvgItem::naturalSizeChanged);
        connect(svg, &Svg::repaintNeeded, this, &SvgItem::elementRectChanged);
        connect(svg, &Svg::sizeChanged, this, &SvgItem::naturalSizeChanged);
        connect(svg, &Svg::sizeChanged, this, &SvgItem::elementRectChanged);
    }

    if (implicitWidth() <= 0) {
        setImplicitWidth(naturalSize().width());
    }
    if (implicitHeight() <= 0) {
        setImplicitHeight(naturalSize().height());
    }

    scheduleImageUpdate();

    Q_EMIT svgChanged();
    Q_EMIT naturalSizeChanged();
    Q_EMIT elementRectChanged();
    Q_EMIT imagePathChanged();
}

KSvg::Svg *SvgItem::svg() const
{
    return m_svg.data();
}

QSizeF SvgItem::naturalSize() const
{
    if (!m_svg) {
        return QSizeF();
    } else if (!m_elementID.isEmpty()) {
        return m_svg->elementSize(m_elementID);
    }

    return m_svg->size();
}

QRectF SvgItem::elementRect() const
{
    if (!m_svg) {
        return QRectF();
    } else if (!m_elementID.isEmpty()) {
        return m_svg->elementRect(m_elementID);
    }

    return QRectF(QPointF(0, 0), m_svg->size());
}

QSGNode *SvgItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *updatePaintNodeData)
{
    Q_UNUSED(updatePaintNodeData);
    if (!window() || !m_svg) {
        delete oldNode;
        return nullptr;
    }

    // this is more than just an optimization, uploading a null image to QSGAtlasTexture causes a crash
    if (width() == 0.0 || height() == 0.0) {
        delete oldNode;
        return nullptr;
    }

    ManagedTextureNode *textureNode = static_cast<ManagedTextureNode *>(oldNode);
    if (!textureNode) {
        textureNode = new ManagedTextureNode;
        m_textureChanged = true;
    }

    // TODO use a heuristic to work out when to redraw
    // if !m_smooth and size is approximate simply change the textureNode.rect without
    // updating the material

    if (m_textureChanged || textureNode->texture()->textureSize() != m_image.size()) {
        // despite having a valid size sometimes we still get a null QImage from KSvg::Svg
        // loading a null texture to an atlas fatals
        // Dave E fixed this in Qt in 5.3.something onwards but we need this for now
        if (m_image.isNull()) {
            delete textureNode;
            return nullptr;
        }

        // The same element at the same size is one picture however many items draw it, and KSvg hands back
        // the same image for it, so one texture is enough: an arrow in sixty list rows was sixty uploads
        // and sixty atlas entries before this.
        textureNode->setTexture(ImageTexturesCache::instance()->loadTexture(window(), m_image, QQuickWindow::TextureCanUseAtlas));
        m_textureChanged = false;

        textureNode->setRect(0, 0, width(), height());
    }

    textureNode->setFiltering(smooth() ? QSGTexture::Linear : QSGTexture::Nearest);

    return textureNode;
}

void SvgItem::updateNeeded()
{
    if (implicitWidth() <= 0) {
        setImplicitWidth(naturalSize().width());
    }
    if (implicitHeight() <= 0) {
        setImplicitHeight(naturalSize().height());
    }
    scheduleImageUpdate();
}

void SvgItem::scheduleImageUpdate()
{
    polish();
    update();
}

void SvgItem::updatePolish()
{
    QQuickItem::updatePolish();

    if (m_svg) {
        // setContainsMultipleImages has to be done there since m_svg can be shared with somebody else
        m_textureChanged = true;
        m_svg->setContainsMultipleImages(!m_elementID.isEmpty());

        // An element which repeats along an axis is the same picture drawn from its own size and stretched
        // by the GPU, which is one small texture rather than one the size of the item. A flat element, a
        // line or a plain fill, repeats along both.
        QSize target(qCeil(width()), qCeil(height()));
        const QSizeF nativeSize = naturalSize();
        if (nativeSize.width() >= 1 && nativeSize.height() >= 1) {
            const RepeatsAlong repeats = repeatsAlong(m_svg, m_elementID, nativeSize);
            if (repeats.horizontally) {
                target.setWidth(qMin(target.width(), qCeil(nativeSize.width())));
            }
            if (repeats.vertically) {
                target.setHeight(qMin(target.height(), qCeil(nativeSize.height())));
            }
        }

        m_image = m_svg->image(target, m_elementID);
    }
}

void SvgItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (newGeometry.size() != oldGeometry.size() && newGeometry.isValid()) {
        scheduleImageUpdate();
    }

    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

void SvgItem::updateDevicePixelRatio()
{
    const auto newDevicePixelRatio = std::max<qreal>(1.0, (window() ? window()->devicePixelRatio() : qApp->devicePixelRatio()));
    if (newDevicePixelRatio != m_svg->devicePixelRatio()) {
        m_svg->setDevicePixelRatio(newDevicePixelRatio);
        m_textureChanged = true;
    }
}

void SvgItem::itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &value)
{
    if (change == ItemSceneChange && value.window) {
        updateDevicePixelRatio();
    } else if (change == QQuickItem::ItemDevicePixelRatioHasChanged) {
        updateDevicePixelRatio();
    }

    QQuickItem::itemChange(change, value);
}

} // KSvg namespace

#include "moc_svgitem.cpp"
