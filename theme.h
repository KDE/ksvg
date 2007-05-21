/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
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

#ifndef PLASMA_THEME_H
#define PLASMA_THEME_H

#include <QtCore/QObject>

#include <plasma_export.h>

namespace Plasma
{

class PLASMA_EXPORT Theme : public QObject
{
    Q_OBJECT

    public:
        /**
         * Singleton pattern accessor
         **/
        static Theme* self();

        explicit Theme( QObject* parent = 0 );
        ~Theme();

        QString themeName() const;
        QString image( const QString& name ) const;

    Q_SIGNALS:
        void changed();

    private:
        class Private;
        Private* const d;
};

} // Plasma namespace

#endif // multiple inclusion guard

