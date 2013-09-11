/*
 *   Copyright 2013 Marco Martin <mart@kde.org>
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

#ifndef CURRENTCONTAINMENTACTIONSMODEL_H
#define CURRENTCONTAINMENTACTIONSMODEL_H

#include <QStandardItemModel>

#include <KConfig>
#include <KConfigGroup>

namespace Plasma {
    class Containment;
    class ContainmentActions;
}

//TODO: out of the library?
class CurrentContainmentActionsModel : public QStandardItemModel
{
    Q_OBJECT

public:
    enum Roles {
        ActionRole = Qt::UserRole+1,
        PluginNameRole,
        HasConfigurationInterfaceRole
    };

    CurrentContainmentActionsModel(Plasma::Containment *cotainment, QObject *parent = 0);
    ~CurrentContainmentActionsModel();

    Q_INVOKABLE bool isTriggerUsed(const QString &trigger);
    Q_INVOKABLE QString mouseEventString(int mouseButtons, int modifiers);
    Q_INVOKABLE QString wheelEventString(const QPointF &delta, int mouseButtons, int modifiers);
    Q_INVOKABLE bool append(const QString &action, const QString &plugin);
    Q_INVOKABLE void update(int row, const QString &action, const QString &plugin);
    Q_INVOKABLE void remove(int row);
    Q_INVOKABLE void showConfiguration(int row);
    Q_INVOKABLE void showAbout(int row);
    Q_INVOKABLE void save();

private:
    Plasma::Containment *m_containment;
    QHash<QString, Plasma::ContainmentActions *> m_plugins;
    KConfigGroup m_baseCfg;
    KConfigGroup m_tempConfig;
    KConfig m_tempConfigParent;
    QStringList m_removedTriggers;
};

#endif
