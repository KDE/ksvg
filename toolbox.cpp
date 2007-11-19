/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License as
 *   published by the Free Software Foundation; either version 2, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "toolbox_p.h"

#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QRadialGradient>

#include "widgets/widget.h"

namespace Plasma
{

DesktopToolbox::DesktopToolbox(QGraphicsItem *parent)
    : QGraphicsItem(parent),
      m_icon("configure"),
      m_size(50),
      m_showing(false),
      m_animId(0),
      m_animFrame(0)
{
    setAcceptsHoverEvents(true);
    setZValue(10000);
    setFlag(ItemClipsToShape, true);
    setFlag(ItemClipsChildrenToShape, false);

    connect(Plasma::Phase::self(), SIGNAL(movementComplete(QGraphicsItem*)), this, SLOT(toolMoved(QGraphicsItem*)));
}

/*QRectF DesktopToolbox::sizeHint() const
{
    return boundingRect();
}*/

QRectF DesktopToolbox::boundingRect() const
{
    return QRectF(0, 0, m_size*2, m_size*2);
}

void DesktopToolbox::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    QPainterPath p = shape();
    QRadialGradient gradient(QPoint(m_size*2, 0), m_size*3);
    gradient.setFocalPoint(QPointF(m_size*2, 0));
    gradient.setColorAt(0, QColor(255, 255, 255, 128));
    gradient.setColorAt(.9, QColor(128, 128, 128, 128));
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(gradient);
    painter->drawPath(p);
    painter->restore();
    m_icon.paint(painter, QRect(m_size*2 - 34, 2, 32, 32));
}

QPainterPath DesktopToolbox::shape() const
{
    QPainterPath path;
    int size = m_size + m_animFrame;
    path.moveTo(m_size*2, 0);
    path.arcTo(QRectF(m_size*2 - size, -size, size*2, size*2), 180, 90);
    return path;
}

void DesktopToolbox::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
//    Plasma::Phase::self()->moveItem(this, Phase::SlideIn, QPoint(-25, -25));
    int x = -25; // pos().x();
    int y = 0; // pos().y();
    Plasma::Phase* phase = Plasma::Phase::self();
    foreach (QGraphicsItem* tool, QGraphicsItem::children()) {
//        kDebug() << "let's show and move" << (QObject*)tool << tool->geometry().toRect();
        tool->show();
        phase->moveItem(tool, Plasma::Phase::SlideIn, QPoint(x, y));
        //x += 0;
        y += static_cast<int>(tool->boundingRect().height()) + 5;
    }

    if (m_animId) {
        phase->stopCustomAnimation(m_animId);
    }

    m_showing = true;
    m_animId = phase->customAnimation(m_size, 150, Plasma::Phase::EaseInCurve, this, "animate");
    QGraphicsItem::hoverEnterEvent(event);
}

void DesktopToolbox::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
//    Plasma::Phase::self->moveItem(this, Phase::SlideOut, boundingRect()QPoint(-50, -50));
    int x = 0; // pos().x() + geometry().width();
    int y = 0;
    Plasma::Phase* phase = Plasma::Phase::self();
    foreach (QGraphicsItem* tool, QGraphicsItem::children()) {
        phase->moveItem(tool, Plasma::Phase::SlideOut, QPoint(x, y));
    }

    if (m_animId) {
        phase->stopCustomAnimation(m_animId);
    }

    m_showing = false;
    m_animId = phase->customAnimation(m_size, 150, Plasma::Phase::EaseOutCurve, this, "animate");
    QGraphicsItem::hoverLeaveEvent(event);
}

void DesktopToolbox::animate(qreal progress)
{
    if (m_showing) {
        m_animFrame = static_cast<int>(m_size * progress);
    } else {
        m_animFrame = static_cast<int>(m_size * (1.0 - progress));
    }

    //kDebug() << "animating at" << progress << "for" << m_animFrame;

    if (progress >= 1) {
        m_animId = 0;
    }

    update();
}

void DesktopToolbox::toolMoved(QGraphicsItem *item)
{
    //kDebug() << "geometry is now " << static_cast<Plasma::Widget*>(item)->geometry();
    if (!m_showing &&
        QGraphicsItem::children().indexOf(static_cast<Plasma::Widget*>(item)) != -1) {
        item->hide();
    }
}

void DesktopToolbox::addTool(QGraphicsItem *tool)
{
    if (!tool) {
        return;
    }

    tool->hide();
    tool->setPos(QPoint(0,0));
    tool->setZValue(zValue() + 1);
    tool->setParentItem(this);
}

} // plasma namespace

#include "toolbox_p.moc"

