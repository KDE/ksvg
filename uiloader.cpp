/*
 *   Copyright 2007 Richard J. Moore <rich@kde.org>
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

#include "uiloader.h"

#include <QStringList>

#include <QGraphicsLinearLayout>

#include "plasma/widgets/checkbox.h"
#include "plasma/widgets/flash.h"
#include "plasma/widgets/icon.h"
#include "plasma/widgets/label.h"
#include "plasma/widgets/pushbutton.h"
#include "plasma/widgets/radiobutton.h"
#include "plasma/widgets/meter.h"

#include "plasma/layouts/flowlayout.h"

namespace Plasma
{

class UiLoader::Private
{
public:
    QStringList widgets;
    QStringList layouts;
};

UiLoader::UiLoader( QObject *parent )
    : d( new Private() )
{
    d->widgets
        << "CheckBox"
        << "Flash"
        << "Icon"
        << "Label"
        << "PushButton"
        << "RadioButton"
        << "Meter";

    d->layouts
        << "VBoxLayout"
        << "HBoxLayout"
        << "FlowLayout";
}

UiLoader::~UiLoader()
{
    delete d;
}

QStringList UiLoader::availableWidgets() const
{
    return d->widgets;
}

Widget *UiLoader::createWidget( const QString &className, Widget *parent )
{
    if (className == QString("CheckBox")) {
        return new CheckBox( parent );
    }
    else if (className == QString("Flash")) {
        return new Flash( parent );
    }
    else if (className == QString("Icon")) {
        return new Icon( parent );
    }
    else if (className == QString("Label")) {
        return new Label( parent ); // Constructor here requires a Widget
    }
    else if (className == QString("PushButton")) {
        return new PushButton( parent ); // Constructor here requires a Widget
    }
    else if (className == QString("RadioButton")) {
        return new RadioButton( parent );
    }
    else if (className == QString("Meter")) {
        return new Meter( parent );
    }

    return 0;
}

QStringList UiLoader::availableLayouts() const
{
    return d->layouts;
}

QGraphicsLayout *UiLoader::createLayout( const QString &className, QGraphicsLayoutItem *parent )
{
    if (className == QString("HBoxLayout")) {
        return new QGraphicsLinearLayout( Qt::Horizontal, parent );
    }
    else if (className == QString("VBoxLayout")) {
        return new QGraphicsLinearLayout( Qt::Vertical, parent );
    }
    else if (className == QString("FlowLayout")) {
        return new FlowLayout( parent );
    }

    return 0;
}

}
