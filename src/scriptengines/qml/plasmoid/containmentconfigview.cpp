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

#include "containmentconfigview.h"
#include "plasmoid/containmentinterface.h"
#include "plasmoid/wallpaperinterface.h"
#include "declarative/configpropertymap.h"

#include <QDebug>
#include <QDir>
#include <QQmlContext>


#include <KLocalizedString>

#include <Plasma/PluginLoader>


//////////////////////////////ContainmentConfigView
ContainmentConfigView::ContainmentConfigView(ContainmentInterface *interface, QWindow *parent)
    : ConfigView(interface, parent),
      m_containmentInterface(interface),
      m_wallpaperConfigModel(0),
      m_currentWallpaperConfig(0)
{
    engine()->rootContext()->setContextProperty("configDialog", this);
    setCurrentWallpaper(interface->containment()->wallpaper());
}

ContainmentConfigView::~ContainmentConfigView()
{
}

ConfigModel *ContainmentConfigView::wallpaperConfigModel()
{
    if (!m_wallpaperConfigModel) {
        m_wallpaperConfigModel = new ConfigModel(this);
        QStringList dirs(QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, "plasma/wallpapers", QStandardPaths::LocateDirectory));
        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
        foreach (const QString &dirPath, dirs) {
            QDir dir(dirPath);
            pkg.setDefaultPackageRoot(dirPath);
            QStringList packages;

            foreach (const QString &sdir, dir.entryList(QDir::AllDirs | QDir::Readable)) {
                QString metadata = dirPath + '/' + sdir + "/metadata.desktop";
                if (QFile::exists(metadata)) {
                    packages << sdir;
                }
            }

            foreach (const QString &package, packages) {
                pkg.setPath(package);
                if (!pkg.isValid()) {
                    continue;
                }
                ConfigCategory *cat = new ConfigCategory(m_wallpaperConfigModel);
                cat->setName(pkg.metadata().name());
                cat->setIcon(pkg.metadata().icon());
                cat->setSource(pkg.filePath("ui", "config.qml"));
                cat->setPluginName(package);
                m_wallpaperConfigModel->appendCategory(cat);
            }
        }
    }
    return m_wallpaperConfigModel;
}

ConfigPropertyMap *ContainmentConfigView::wallpaperConfiguration() const
{
    return m_currentWallpaperConfig;
}

QString ContainmentConfigView::currentWallpaper() const
{
    return m_currentWallpaper;
}

void ContainmentConfigView::setCurrentWallpaper(const QString &wallpaper)
{
    if (m_currentWallpaper == wallpaper) {
        return;
    }

    if (m_containmentInterface->containment()->wallpaper() == wallpaper) {
        delete m_currentWallpaperConfig;
        if (m_containmentInterface->wallpaperInterface()) {
            m_currentWallpaperConfig = m_containmentInterface->wallpaperInterface()->configuration();
        }
    } else {
        if (m_containmentInterface->containment()->wallpaper() != m_currentWallpaper) {
            delete m_currentWallpaperConfig;
        }

        //we have to construct an independent ConfigPropertyMap when we want to configure wallpapers that are not the current one
        Plasma::Package pkg = Plasma::PluginLoader::self()->loadPackage("Plasma/Generic");
        pkg.setDefaultPackageRoot("plasma/wallpapers");
        pkg.setPath(wallpaper);
        QFile file(pkg.filePath("config", "main.xml"));
        KConfigGroup cfg = m_containmentInterface->containment()->config();
        cfg = KConfigGroup(&cfg, "Wallpaper");
        m_currentWallpaperConfig = new ConfigPropertyMap(new Plasma::ConfigLoader(&cfg, &file), this);
    }

    m_currentWallpaper = wallpaper;
    emit currentWallpaperChanged();
    emit wallpaperConfigurationChanged();
}

void ContainmentConfigView::applyWallpaper()
{
    m_containmentInterface->containment()->setWallpaper(m_currentWallpaper);

    if (m_currentWallpaperConfig != m_containmentInterface->wallpaperInterface()->configuration()) {
        delete m_currentWallpaperConfig;
        m_currentWallpaperConfig = m_containmentInterface->wallpaperInterface()->configuration();
        emit wallpaperConfigurationChanged();
    }
}

#include "moc_containmentconfigview.cpp"
