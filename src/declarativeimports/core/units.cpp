/***************************************************************************
 *   Copyright 2013 Marco Martin <mart@kde.org            >                *
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

#include "units.h"

#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QtGlobal>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <cmath>

#include <KDirWatch>
#include <KIconLoader>

const QString plasmarc = QStringLiteral("plasmarc");
const QString groupName = QStringLiteral("Units");
const int defaultLongDuration = 250;

Units::Units (QObject *parent)
    : QObject(parent),
      m_gridUnit(-1),
      m_devicePixelRatio(-1),
      m_longDuration(defaultLongDuration) // default base value for animations
{
    m_iconSizes = new QQmlPropertyMap(this);
    updateDevicePixelRatio();
    updateSpacing();

    //iconLoaderSettingsChanged();

    connect(KIconLoader::global(), SIGNAL(iconLoaderSettingsChanged()), this, SLOT(iconLoaderSettingsChanged()));

    themeChanged();
    connect(&m_theme, SIGNAL(themeChanged()),
            this, SLOT(themeChanged()));
    installEventFilter(qApp);

    const QString configFile = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/') + plasmarc;
    KDirWatch::self()->addFile(configFile);

    // Catch both, direct changes to the config file ...
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &Units::settingsFileChanged);
    // ... but also remove/recreate cycles, like KConfig does it
    connect(KDirWatch::self(), &KDirWatch::created, this, &Units::settingsFileChanged);
    // Trigger configuration read
    settingsFileChanged(plasmarc);
}

Units::~Units()
{
}

void Units::settingsFileChanged(const QString &file)
{
    if (file.endsWith(plasmarc)) {

        KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(plasmarc), groupName);
        cfg.config()->reparseConfiguration();
        const int longDuration = cfg.readEntry("longDuration", defaultLongDuration);

        if (longDuration != m_longDuration) {
            m_longDuration = longDuration;
            emit durationChanged();
        }
    }
}

void Units::iconLoaderSettingsChanged()
{
    // These are not scaled, we respect the user's setting over dpi scaling
    m_iconSizes->insert("desktop", QVariant(KIconLoader::global()->currentSize(KIconLoader::Desktop)));

    // The following icon sizes are fully scaled to dpi
    m_iconSizes->insert("small", devicePixelIconSize(KIconLoader::SizeSmall));
    m_iconSizes->insert("smallMedium", devicePixelIconSize(KIconLoader::SizeSmallMedium));
    m_iconSizes->insert("medium", devicePixelIconSize(KIconLoader::SizeMedium));
    m_iconSizes->insert("large", devicePixelIconSize(KIconLoader::SizeLarge));
    m_iconSizes->insert("huge", devicePixelIconSize(KIconLoader::SizeHuge));
    m_iconSizes->insert("enormous", devicePixelIconSize(KIconLoader::SizeEnormous));

    emit iconSizesChanged();
}

QQmlPropertyMap *Units::iconSizes() const
{
    return m_iconSizes;
}

int Units::devicePixelIconSize(const int size) const
{
    /* in kiconloader.h
    enum StdSizes {
        SizeSmall=16,
        SizeSmallMedium=22,
        SizeMedium=32,
        SizeLarge=48,
        SizeHuge=64,
        SizeEnormous=128
    };
    */
    // Scale the icon sizes up using the devicePixelRatio
    // This function returns the next stepping icon size
    // and multiplies the global settings with the dpi ratio.
    const int dpisize = devicePixelRatio() * size;
    int out = KIconLoader::SizeSmall;
    if (devicePixelRatio() < 1.5) {
        return size;
    } else if (devicePixelRatio() < 2.0) {
        out = size * 1.5;
    } else if (devicePixelRatio() < 2.5) {
        out = size * 2.0;
    } else if (devicePixelRatio() < 3.0) {
        out = size * 3.0;
    } else {
        out = dpisize;
    }
    // FIXME: Add special casing for < 64 cases: align to kiconloader size

    //qDebug() << " Size in: " << size << dpisize << " -> " << out;
    return out;
}

qreal Units::devicePixelRatio() const
{
    return m_devicePixelRatio;
}

void Units::updateDevicePixelRatio()
{
    // Going through QDesktopWidget seems to be the most reliable way no
    // to get the DPI, and thus devicePixelRatio
    // Using QGuiApplication::devicePixelRatio() gives too coarse values,
    // i.e. it directly jumps from 1.0 to 2.0. We want tighter control on
    // sizing, so we compute the exact ratio and use that.
    qreal dpi = QApplication::desktop()->physicalDpiX();
    // Usual "default" is 96 dpi
    // that magic ratio follows the definition of "device independent pixel" by Microsoft
    m_devicePixelRatio = (qreal)dpi / (qreal)96;
    iconLoaderSettingsChanged();
    emit devicePixelRatioChanged();
}

int Units::gridUnit() const
{
    return m_gridUnit;
}

void Units::themeChanged()
{
    const int gridUnit = QFontMetrics(QApplication::font()).boundingRect("M").height();
    if (gridUnit != m_gridUnit) {
        m_gridUnit = gridUnit;
        emit gridUnitChanged();
    }
}

int Units::smallSpacing() const
{
    return m_smallSpacing;
}

int Units::largeSpacing() const
{
    return m_largeSpacing;
}

void Units::updateSpacing()
{
    const int _s = QFontMetrics(QApplication::font()).boundingRect("M").size().height();
    if (_s != m_largeSpacing) {
        m_smallSpacing = qMax(2, (int)(_s / 8)); // 1/8 of msize.height, at least 2
        m_largeSpacing = _s; // msize.height
        emit spacingChanged();
    }
}


int Units::longDuration() const
{
    return m_longDuration;
}

int Units::shortDuration() const
{
    return m_longDuration / 5;
}

bool Units::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QCoreApplication::instance()) {
        if (event->type() == QEvent::ApplicationFontChange || event->type() == QEvent::FontChange) {
            updateSpacing();
        }
    }
    return QObject::eventFilter(watched, event);
}

