/*
 *   Copyright 2007 by André Duffeck <duffeck@kde.org>
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

#ifndef FLASH_H_
#define FLASH_H_

#include <QtCore/QObject>
#include <QtGui/QGraphicsItem>
#include <QtGui/QTextOption>

#include <plasma/plasma_export.h>
#include <plasma/widgets/widget.h>

namespace Plasma
{

/**
 * Class that allows to flash text or icons inside plasma
 */
class PLASMA_EXPORT Flash : public Plasma::Widget
{
    Q_OBJECT
    public:
        Flash(QGraphicsItem *parent = 0);
        virtual ~Flash();

        void paintWidget(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);
        QRectF boundingRect() const;

        void setFont( const QFont & );
        void setColor( const QColor & );
        void setDuration( int duration );

        void flash( const QString &text, int duration = 0, const QTextOption &option = QTextOption(Qt::AlignCenter) );
        void flash( const QPixmap &pixmap, int duration = 0, Qt::Alignment align = Qt::AlignCenter );

    public Q_SLOTS:
        void kill();

    protected Q_SLOTS:
        void fadeIn();
        void fadeOut();

    protected:
        QPixmap renderPixmap();

    private:
        class Private;
        Private * const d;
};

}

#endif
