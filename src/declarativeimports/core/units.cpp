/*
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 Sebastian Kügler <sebas@kde.org>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmunsdon@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "units.h"

#include <QGuiApplication>
#include <QDebug>
#include <QtGlobal>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>
#include <QFontMetrics>
#include <cmath>

#include <KIconLoader>

const int defaultLongDuration = 250;


SharedAppFilter::SharedAppFilter(QObject *parent)
    : QObject(parent)
{
    QCoreApplication::instance()->installEventFilter(this);
}

SharedAppFilter::~SharedAppFilter()
{}

bool SharedAppFilter::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QCoreApplication::instance()) {
        if (event->type() == QEvent::ApplicationFontChange) {
            emit fontChanged();
        }
    }
    return QObject::eventFilter(watched, event);
}

SharedAppFilter *Units::s_sharedAppFilter = nullptr;

Units::Units(QObject *parent)
    : QObject(parent),
      m_gridUnit(-1),
      m_devicePixelRatio(-1),
      m_smallSpacing(-1),
      m_largeSpacing(-1),
      m_longDuration(defaultLongDuration) // default base value for animations
{
    if (!s_sharedAppFilter) {
        s_sharedAppFilter = new SharedAppFilter();
    }

    m_iconSizes = new QQmlPropertyMap(this);
    m_iconSizeHints = new QQmlPropertyMap(this);
    updateDevicePixelRatio(); // also updates icon sizes
    updateSpacing(); // updates gridUnit and *Spacing properties

    connect(KIconLoader::global(), &KIconLoader::iconLoaderSettingsChanged, this, &Units::iconLoaderSettingsChanged);
    QObject::connect(s_sharedAppFilter, &SharedAppFilter::fontChanged, this, &Units::updateSpacing);

    m_animationSpeedWatcher = KConfigWatcher::create(KSharedConfig::openConfig());
    connect(m_animationSpeedWatcher.data(), &KConfigWatcher::configChanged, this,
        [this](const KConfigGroup &group, const QByteArrayList &names) {
            if (group.name() == QLatin1String("KDE") && names.contains(QByteArrayLiteral("AnimationDurationFactor"))) {
                updateAnimationSpeed();
            }
    });
    updateAnimationSpeed();
}

Units::~Units()
{

}

Units &Units::instance()
{
    static Units units;
    return units;
}

void Units::updateAnimationSpeed()
{
    KConfigGroup generalCfg = KConfigGroup(KSharedConfig::openConfig(), QStringLiteral("KDE"));
    const qreal animationSpeedModifier = qMax(0.0, generalCfg.readEntry("AnimationDurationFactor", 1.0));

    // Read the old longDuration value for compatibility
    KConfigGroup cfg = KConfigGroup(KSharedConfig::openConfig(QStringLiteral("plasmarc")), QStringLiteral("Units"));
    int longDuration = cfg.readEntry("longDuration", defaultLongDuration);

    longDuration = qRound(longDuration * animationSpeedModifier);

    // Animators with a duration of 0 do not fire reliably
    // see Bug 357532 and QTBUG-39766
    longDuration = qMax(1, longDuration);

    if (longDuration != m_longDuration) {
        m_longDuration = longDuration;
        emit durationChanged();
    }
}


void Units::iconLoaderSettingsChanged()
{
    m_iconSizes->insert(QStringLiteral("desktop"), devicePixelIconSize(KIconLoader::global()->currentSize(KIconLoader::Desktop)));

    m_iconSizes->insert(QStringLiteral("tiny"), devicePixelIconSize(KIconLoader::SizeSmall) / 2);
    m_iconSizes->insert(QStringLiteral("small"), devicePixelIconSize(KIconLoader::SizeSmall));
    m_iconSizes->insert(QStringLiteral("smallMedium"), devicePixelIconSize(KIconLoader::SizeSmallMedium));
    m_iconSizes->insert(QStringLiteral("medium"), devicePixelIconSize(KIconLoader::SizeMedium));
    m_iconSizes->insert(QStringLiteral("large"), devicePixelIconSize(KIconLoader::SizeLarge));
    m_iconSizes->insert(QStringLiteral("huge"), devicePixelIconSize(KIconLoader::SizeHuge));
    m_iconSizes->insert(QStringLiteral("enormous"), devicePixelIconSize(KIconLoader::SizeEnormous));

    m_iconSizeHints->insert(QStringLiteral("panel"), devicePixelIconSize(KIconLoader::global()->currentSize(KIconLoader::Panel)));
    m_iconSizeHints->insert(QStringLiteral("desktop"), devicePixelIconSize(KIconLoader::global()->currentSize(KIconLoader::Desktop)));

    emit iconSizesChanged();
    emit iconSizeHintsChanged();
}

QQmlPropertyMap *Units::iconSizes() const
{
    return m_iconSizes;
}

QQmlPropertyMap *Units::iconSizeHints() const
{
    return m_iconSizeHints;
}

int Units::roundToIconSize(int size)
{
    /*Do *not* use devicePixelIconSize here, we want to use the sizes of the pixmaps of the smallest icons on the disk. And those are unaffected by dpi*/
    if (size <= 0) {
        return 0;
    } else if (size < KIconLoader::SizeSmall) {
        return KIconLoader::SizeSmall/2;
    } else if (size < KIconLoader::SizeSmallMedium) {
        return KIconLoader::SizeSmall;

    } else if (size < KIconLoader::SizeMedium) {
        return KIconLoader::SizeSmallMedium;

    } else if (size < KIconLoader::SizeLarge) {
        return KIconLoader::SizeMedium;

    } else if (size < KIconLoader::SizeHuge) {
        return KIconLoader::SizeLarge;

    } else {
        return size;
    }
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
    const qreal ratio = devicePixelRatio();

    if (ratio < 1.5) {
        return size;
    } else if (ratio < 2.0) {
        return size * 1.5;
    } else if (ratio < 2.5) {
        return size * 2.0;
    } else if (ratio < 3.0) {
        return size * 2.5;
    } else if (ratio < 3.5) {
        return size * 3.0;
    } else {
        return size * ratio;
    }
    // FIXME: Add special casing for < 64 cases: align to kiconloader size
}

qreal Units::devicePixelRatio() const
{
    return m_devicePixelRatio;
}

void Units::updateDevicePixelRatio()
{
    // Using QGuiApplication::devicePixelRatio() gives too coarse values,
    // i.e. it directly jumps from 1.0 to 2.0. We want tighter control on
    // sizing, so we compute the exact ratio and use that.
    // TODO: make it possible to adapt to the dpi for the current screen dpi
    //  instead of assuming that all of them use the same dpi which applies for
    //  X11 but not for other systems.
    QScreen *primary = QGuiApplication::primaryScreen();
    if (!primary) {
        return;
    }
    const qreal dpi = primary->logicalDotsPerInchX();
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
    int gridUnit = QFontMetrics(QGuiApplication::font()).boundingRect(QStringLiteral("M")).height();

    if (gridUnit % 2 != 0) {
        gridUnit++;
    }
    if (gridUnit != m_gridUnit) {
        m_gridUnit = gridUnit;
        emit gridUnitChanged();
    }

    if (gridUnit != m_largeSpacing) {
        m_smallSpacing = qMax(2, (int)(gridUnit / 4)); // 1/4 of gridUnit, at least 2
        m_largeSpacing = gridUnit; // msize.height
        emit spacingChanged();
    }
}

int Units::longDuration() const
{
    return m_longDuration;
}

int Units::shortDuration() const
{
    return qMax(1, qRound(m_longDuration * 0.6));
}

int Units::veryShortDuration() const
{
	return qRound(m_longDuration * 0.3);
}

int Units::veryLongDuration() const
{
    return m_longDuration * 2;
}

#include "moc_units.cpp"

