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

#ifndef PLASMA_DATASOURCE_H
#define PLASMA_DATASOURCE_H

#include <QtCore/QHash>
#include <QtCore/QObject>

#include <plasma_export.h>
#include <dataengine.h>

namespace Plasma
{

class PLASMA_EXPORT DataSource : public QObject
{
    Q_OBJECT

    public:
        typedef QHash<QString, DataSource*> Dict;
        typedef QHash<QString, Dict> Grouping;

        explicit DataSource(QObject* parent = 0);
        virtual ~DataSource();

        QString name();
        void setName(const QString&);
        const DataEngine::Data data() const;
        void setData(const QString& key, const QVariant& value);

        void checkForUpdate();

    Q_SIGNALS:
        void updated(const QString& source, const Plasma::DataEngine::Data& data);

    private:
        class Private;
        Private* const d;
};

} // Plasma namespace

#endif // multiple inclusion guard
