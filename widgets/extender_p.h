/***************************************************************************
 *   Copyright 2008 by Rob Scheepmaker <r.scheepmaker@student.utwente.nl>  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/

#ifndef LIBS_PLASMA_EXTENDER_P_H
#define LIBS_PLASMA_EXTENDER_P_H

#include <QString>
#include <QList>
#include <QPointF>

class QGraphicsGridLayout;
class QGraphicsLinearLayout;
class QGraphicsWidget;

namespace Plasma
{

class Applet;
class Extender;
class ExtenderItem;
class Label;

class ExtenderPrivate
{
    public:
        ExtenderPrivate(Applet *applet, Extender *q);
        ~ExtenderPrivate();

        void addExtenderItem(ExtenderItem *item, const QPointF &pos = QPointF(-1, -1));
        void removeExtenderItem(ExtenderItem *item);
        void adjustSizeHints();
        int insertIndexFromPos(const QPointF &pos) const;
        void loadExtenderItems();

        Extender *q;

        Applet *applet;
        QGraphicsLinearLayout *layout;

        int currentSpacerIndex;
        QGraphicsWidget *spacerWidget;

        QString emptyExtenderMessage;
        Label *emptyExtenderLabel;

        uint sourceAppletId;

        QList<ExtenderItem*> attachedExtenderItems;

        bool popup;

        static QGraphicsGridLayout *s_popupLayout;
};

} // namespace Plasma

#endif // LIBS_PLASMA_EXTENDER_P_H
