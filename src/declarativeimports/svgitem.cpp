/*
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "svgitem.h"

#include <QDebug>
#include <QQuickWindow>
#include <QRectF>
#include <QSGTexture>

#include "ksvg/svg.h"

#include "managedtexturenode.h"

#include <cmath> //floor()

#include <Kirigami/PlatformTheme>

namespace KSvg
{
SvgItem::SvgItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_textureChanged(false)
{
    m_svg = new KSvg::Svg(this);
    setFlag(QQuickItem::ItemHasContents, true);

    connect(m_svg, &Svg::repaintNeeded, this, &SvgItem::updateNeeded);
    connect(m_svg, &Svg::repaintNeeded, this, &SvgItem::naturalSizeChanged);
    connect(m_svg, &Svg::sizeChanged, this, &SvgItem::naturalSizeChanged);
}

SvgItem::~SvgItem()
{
}

void SvgItem::componentComplete()
{
    m_kirigamiTheme = qobject_cast<Kirigami::PlatformTheme *>(qmlAttachedPropertiesObject<Kirigami::PlatformTheme>(this, true));

    auto applyTheme = [this]() {
        m_svg->setPalette(m_kirigamiTheme->palette());
        m_svg->setExtraColor(Svg::Positive, m_kirigamiTheme->positiveTextColor());
        m_svg->setExtraColor(Svg::Neutral, m_kirigamiTheme->neutralTextColor());
        m_svg->setExtraColor(Svg::Negative, m_kirigamiTheme->negativeTextColor());
    };
    applyTheme();
    connect(m_kirigamiTheme, &Kirigami::PlatformTheme::colorsChanged, this, applyTheme);

    QQuickItem::componentComplete();
}

void SvgItem::setImagePath(const QString &path)
{
    if (m_svg->imagePath() == path) {
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

    scheduleImageUpdate();
}

QString SvgItem::elementId() const
{
    return m_elementID;
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

    if (m_textureChanged || textureNode->texture()->textureSize() != QSize(width(), height())) {
        // despite having a valid size sometimes we still get a null QImage from KSvg::Svg
        // loading a null texture to an atlas fatals
        // Dave E fixed this in Qt in 5.3.something onwards but we need this for now
        if (m_image.isNull()) {
            delete textureNode;
            return nullptr;
        }

        QSharedPointer<QSGTexture> texture(window()->createTextureFromImage(m_image, QQuickWindow::TextureCanUseAtlas));
        textureNode->setTexture(texture);
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

void SvgItem::updateDevicePixelRatio()
{
    if (m_svg) {
        // devicepixelratio is always set integer in the svg, so needs at least 192dpi to double up.
        //(it needs to be integer to have lines contained inside a svg piece to keep being pixel aligned)
        if (window()) {
            m_svg->setDevicePixelRatio(qMax<qreal>(1.0, std::ceil(window()->devicePixelRatio())));
        } else {
            m_svg->setDevicePixelRatio(qMax<qreal>(1.0, std::ceil(qApp->devicePixelRatio())));
        }
        // TODO: remove scalefactor
        m_svg->setScaleFactor(1.0);
    }
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
        m_image = m_svg->image(QSize(width(), height()), m_elementID);
    }
}

void SvgItem::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry)
{
    if (newGeometry.size() != oldGeometry.size() && newGeometry.isValid()) {
        scheduleImageUpdate();
    }

    QQuickItem::geometryChange(newGeometry, oldGeometry);
}

} // KSvg namespace
