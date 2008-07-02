/*
 *   Copyright 2005 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2008 by Andrew Lake <jamboarder@yahoo.com>
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

#include <imageeffects.h>
#include "effects/blur.cpp"
#include <QImage>
#include <QPainter>
#include <QPixmap>

namespace Plasma
{
    
namespace ImageEffects
{

void shadowBlur(QImage &image, int radius, const QColor &color)
{
    if (radius < 1)
        return;

    expblur<16, 7>(image, radius);

    QPainter p(&image);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(image.rect(), color);
}

QPixmap shadowText(QString text, QColor textColor, QColor shadowColor, QPoint offset, int radius)
{
    // Draw text 
    QPixmap pixmap(10,10);
    QPainter p(&pixmap);
    QRect textRect = p.fontMetrics().boundingRect(text);
    p.end();
    QPixmap textPixmap(textRect.size());
    textPixmap.fill(Qt::transparent);
    p.begin(&textPixmap);
    p.setPen(textColor);
    p.drawText(textPixmap.rect(), Qt::AlignLeft, text);
    p.end();

    //Draw blurred shadow
    QImage img(textRect.size() + QSize(radius * 2, radius * 2),
    QImage::Format_ARGB32_Premultiplied);
    img.fill(Qt::transparent);
    p.begin(&img);
    p.drawImage(QPoint(0,0), textPixmap.toImage());
    p.end();
    shadowBlur(img, radius, shadowColor);

    //Compose text and shadow
    QPixmap finalPixmap(img.size() + QSize(offset.x(), offset.y()));
    finalPixmap.fill(Qt::transparent);
    p.begin(&finalPixmap);
    p.drawImage(finalPixmap.rect().topLeft() + QPoint(offset.x(), offset.y()), img);
    p.drawPixmap(finalPixmap.rect().topLeft(), textPixmap);
    p.end();

    return finalPixmap;
}

} //ImageEffects namespace

} // Plasma namespace

