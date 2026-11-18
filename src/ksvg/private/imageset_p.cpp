/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageset_p.h"
#include "debug_p.h"
#include "framesvg.h"
#include "framesvg_p.h"
#include "svg_p.h"

#include <QDir>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QMetaEnum>
#include <QThread>

#include <KColorUtils>
#include <KSharedConfig>
#include <kpluginmetadata.h>

namespace KSvg
{
const char ImageSetPrivate::defaultImageSet[] = "default";

ImageSetPrivate *ImageSetPrivate::globalImageSet = nullptr;
QHash<QString, ImageSetPrivate *> ImageSetPrivate::themes = QHash<QString, ImageSetPrivate *>();
using QSP = QStandardPaths;

#if KSVG_BUILD_DEPRECATED_SINCE(6, 21)
KPluginMetaData metaDataForImageSet(const QString &basePath, const QString &theme)
{
    QString packageBasePath = basePath % theme;
    QDir dir;
    if (!dir.exists(packageBasePath)) {
        packageBasePath = QSP::locate(QSP::GenericDataLocation, basePath % theme, QSP::LocateDirectory);
    }
    if (packageBasePath.isEmpty()) {
        qWarning(LOG_KSVG) << "Could not locate KSvg image set" << theme << "in" << basePath << "using search path"
                           << QSP::standardLocations(QSP::GenericDataLocation);
        return {};
    }
    if (QFileInfo::exists(packageBasePath + QLatin1String("/metadata.json"))) {
        return KPluginMetaData::fromJsonFile(packageBasePath + QLatin1String("/metadata.json"));
    } else if (QFileInfo::exists(packageBasePath + QLatin1String("/metadata.desktop"))) {
        QString metadataPath = packageBasePath + QLatin1String("/metadata.desktop");
        KConfigGroup cg(KSharedConfig::openConfig(packageBasePath + QLatin1String("/metadata.desktop"), KConfig::SimpleConfig),
                        QStringLiteral("Desktop Entry"));
        QJsonObject obj = {};
        for (const QString &key : cg.keyList()) {
            obj[key] = cg.readEntry(key);
        }
        qWarning(LOG_KSVG) << "The theme" << theme
                           << "uses the legacy metadata.desktop. Consider contacting the author and asking them update it to use the newer JSON format.";
        return KPluginMetaData(obj, packageBasePath + QLatin1String("/metadata.desktop"));
    } else {
        qCWarning(LOG_KSVG) << "Could not locate metadata for theme" << theme;
        return {};
    }
}
#endif

ImageSetPrivate::ImageSetPrivate(const QString &_basePath, QObject *parent)
    : QObject(parent)
    , basePath(_basePath)
    , colorScheme(QPalette::Active, KColorScheme::Window, KSharedConfigPtr(nullptr))
    , selectionColorScheme(QPalette::Active, KColorScheme::Selection, KSharedConfigPtr(nullptr))
    , buttonColorScheme(QPalette::Active, KColorScheme::Button, KSharedConfigPtr(nullptr))
    , viewColorScheme(QPalette::Active, KColorScheme::View, KSharedConfigPtr(nullptr))
    , complementaryColorScheme(QPalette::Active, KColorScheme::Complementary, KSharedConfigPtr(nullptr))
    , headerColorScheme(QPalette::Active, KColorScheme::Header, KSharedConfigPtr(nullptr))
    , tooltipColorScheme(QPalette::Active, KColorScheme::Tooltip, KSharedConfigPtr(nullptr))
    , cachesToDiscard(NoCache)
#if KSVG_BUILD_DEPRECATED_SINCE(6, 21)
    , useGlobal(true)
#endif
    , cacheImageSet(true)
{
    if (basePath.isEmpty()) {
        const QString org = QCoreApplication::organizationName();
        if (!org.isEmpty()) {
            basePath += u'/' + org;
        }
        const QString appName = QCoreApplication::applicationName();
        if (!appName.isEmpty()) {
            basePath += u'/' + appName;
        }
        if (basePath.isEmpty()) {
            basePath = QStringLiteral("ksvg");
        }
        basePath += u"/svgtheme/";
    }

    pixmapSaveTimer = new QTimer(this);
    pixmapSaveTimer->setSingleShot(true);
    pixmapSaveTimer->setInterval(600);
    QObject::connect(pixmapSaveTimer, &QTimer::timeout, this, &ImageSetPrivate::scheduledCacheUpdate);

    updateNotificationTimer = new QTimer(this);
    updateNotificationTimer->setSingleShot(true);
    updateNotificationTimer->setInterval(100);
    QObject::connect(updateNotificationTimer, &QTimer::timeout, this, &ImageSetPrivate::notifyOfChanged);

    QCoreApplication::instance()->installEventFilter(this);
}

ImageSetPrivate::~ImageSetPrivate()
{
    FrameSvgPrivate::s_sharedFrames.remove(this);
}

bool ImageSetPrivate::useCache()
{
    return cacheImageSet;
}

void ImageSetPrivate::onAppExitCleanup()
{
    pixmapsToCache.clear();
    keysToCache.clear();
    idsToCache.clear();
    pixmapSaveTimer->stop();
    cacheImageSet = false;
}

QString ImageSetPrivate::imagePath(const QString &theme, const QString &type, const QString &image)
{
    QString subdir = basePath % theme % type % image;

    if (QFileInfo::exists(subdir)) {
        return subdir;
    } else {
        return QStandardPaths::locate(QStandardPaths::GenericDataLocation, subdir);
    }
}

QString ImageSetPrivate::findInImageSet(const QString &image, const QString &theme, bool cache)
{
    if (cache) {
        auto it = discoveries.constFind(image);
        if (it != discoveries.constEnd()) {
            return it.value();
        }
    }

    QString search;

    // TODO: use also QFileSelector::allSelectors?
    // TODO: check if the theme supports selectors starting with +
    for (const QString &type : std::as_const(selectors)) {
        search = imagePath(theme, QLatin1Char('/') % type % QLatin1Char('/'), image);
        if (!search.isEmpty()) {
            break;
        }
    }

    // not found in selectors
    if (search.isEmpty()) {
        search = imagePath(theme, QStringLiteral("/"), image);
    }

    if (cache && !search.isEmpty()) {
        discoveries.insert(image, search);
    }

    return search;
}

void ImageSetPrivate::discardCache(CacheTypes caches)
{
    if (caches & PixmapCache) {
        pixmapsToCache.clear();
        pixmapSaveTimer->stop();
        for (const QString &key : std::as_const(cachedPixmapKeys)) {
            QPixmapCache::remove(QLatin1String("ksvg:") + key);
        }
        cachedPixmapKeys.clear();
        keysToCache.clear();
        idsToCache.clear();
    }

    cachedSvgStyleSheets.clear();
    cachedSelectedSvgStyleSheets.clear();
    cachedInactiveSvgStyleSheets.clear();

    if (caches & SvgElementsCache) {
        discoveries.clear();
        hasColorsKnown = false;
    }
}

void ImageSetPrivate::scheduledCacheUpdate()
{
    if (useCache()) {
        QHashIterator<QString, QPixmap> it(pixmapsToCache);
        while (it.hasNext()) {
            it.next();
            const QString key = idsToCache.value(it.key());
            if (QPixmapCache::insert(QLatin1String("ksvg:") + key, it.value())) {
                cachedPixmapKeys.insert(key);
            }
        }
    }

    pixmapsToCache.clear();
    keysToCache.clear();
    idsToCache.clear();
}

void ImageSetPrivate::colorsChanged()
{
    // in the case the theme follows the desktop settings, refetch the colorschemes
    // and discard the svg pixmap cache
    if (!colors) {
        KSharedConfig::openConfig()->reparseConfiguration();
    }
    colorScheme = KColorScheme(QPalette::Active, KColorScheme::Window, colors);
    buttonColorScheme = KColorScheme(QPalette::Active, KColorScheme::Button, colors);
    viewColorScheme = KColorScheme(QPalette::Active, KColorScheme::View, colors);
    selectionColorScheme = KColorScheme(QPalette::Active, KColorScheme::Selection, colors);
    complementaryColorScheme = KColorScheme(QPalette::Active, KColorScheme::Complementary, colors);
    headerColorScheme = KColorScheme(QPalette::Active, KColorScheme::Header, colors);
    tooltipColorScheme = KColorScheme(QPalette::Active, KColorScheme::Tooltip, colors);
    scheduleImageSetChangeNotification(PixmapCache | SvgElementsCache);
}

void ImageSetPrivate::scheduleImageSetChangeNotification(CacheTypes caches)
{
    cachesToDiscard |= caches;
    updateNotificationTimer->start();
}

void ImageSetPrivate::notifyOfChanged()
{
    // qCDebug(LOG_KSVG) << cachesToDiscard;
    discardCache(cachesToDiscard);
    cachesToDiscard = NoCache;
    Q_EMIT imageSetChanged(imageSetName);
}

QColor ImageSetPrivate::namedColor(Svg::StyleSheetColor colorName, const KSvg::Svg *svg)
{
    const KSvg::Svg::Status status = svg->status();
    KColorScheme *currentScheme = nullptr;

    switch (KColorScheme::ColorSet(svg->colorSet())) {
    case KColorScheme::Button:
        currentScheme = &buttonColorScheme;
        break;
    case KColorScheme::View:
        currentScheme = &viewColorScheme;
        break;
    case KColorScheme::Complementary:
        currentScheme = &complementaryColorScheme;
        break;
    case KColorScheme::Header:
        currentScheme = &headerColorScheme;
        break;
    case KColorScheme::Tooltip:
        currentScheme = &tooltipColorScheme;
        break;
    default:
        currentScheme = &colorScheme;
    }

    switch (colorName) {
    case Svg::Text:
        switch (status) {
        case Svg::Status::Selected:
            return selectionColorScheme.foreground(KColorScheme::NormalText).color();
        case Svg::Status::Inactive:
            return currentScheme->foreground(KColorScheme::InactiveText).color();
        default:
            return currentScheme->foreground(KColorScheme::NormalText).color();
        }
    case Svg::Background:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.background(KColorScheme::NormalBackground).color();
        } else {
            return colorScheme.background(KColorScheme::NormalBackground).color();
        }
    case Svg::Highlight:
        return selectionColorScheme.background(KColorScheme::NormalBackground).color();
    case Svg::HighlightedText:
        return selectionColorScheme.foreground(KColorScheme::NormalText).color();
    case Svg::PositiveText:
        return currentScheme->foreground(KColorScheme::PositiveText).color();
    case Svg::NeutralText:
        return currentScheme->foreground(KColorScheme::NeutralText).color();
    case Svg::NegativeText:
        return currentScheme->foreground(KColorScheme::NegativeText).color();

    case Svg::ButtonText:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.foreground(KColorScheme::NormalText).color();
        } else {
            return buttonColorScheme.foreground(KColorScheme::NormalText).color();
        }
    case Svg::ButtonBackground:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.background(KColorScheme::NormalBackground).color();
        } else {
            return buttonColorScheme.background(KColorScheme::NormalBackground).color();
        }
    case Svg::ButtonHover:
        return buttonColorScheme.decoration(KColorScheme::HoverColor).color();
    case Svg::ButtonFocus:
        return buttonColorScheme.decoration(KColorScheme::FocusColor).color();
    case Svg::ButtonHighlightedText:
        return selectionColorScheme.foreground(KColorScheme::NormalText).color();
    case Svg::ButtonPositiveText:
        return buttonColorScheme.foreground(KColorScheme::PositiveText).color();
    case Svg::ButtonNeutralText:
        return buttonColorScheme.foreground(KColorScheme::NeutralText).color();
    case Svg::ButtonNegativeText:
        return buttonColorScheme.foreground(KColorScheme::NegativeText).color();

    case Svg::ViewText:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.foreground(KColorScheme::NormalText).color();
        } else {
            return viewColorScheme.foreground(KColorScheme::NormalText).color();
        }
    case Svg::ViewBackground:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.background(KColorScheme::NormalBackground).color();
        } else {
            return viewColorScheme.background(KColorScheme::NormalBackground).color();
        }
    case Svg::ViewHover:
        return viewColorScheme.decoration(KColorScheme::HoverColor).color();
    case Svg::ViewFocus:
        return viewColorScheme.decoration(KColorScheme::FocusColor).color();
    case Svg::ViewHighlightedText:
        return selectionColorScheme.foreground(KColorScheme::NormalText).color();
    case Svg::ViewPositiveText:
        return viewColorScheme.foreground(KColorScheme::PositiveText).color();
    case Svg::ViewNeutralText:
        return viewColorScheme.foreground(KColorScheme::NeutralText).color();
    case Svg::ViewNegativeText:
        return viewColorScheme.foreground(KColorScheme::NegativeText).color();

    case Svg::TooltipText:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.foreground(KColorScheme::NormalText).color();
        } else {
            return tooltipColorScheme.foreground(KColorScheme::NormalText).color();
        }
    case Svg::TooltipBackground:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.background(KColorScheme::NormalBackground).color();
        } else {
            return tooltipColorScheme.background(KColorScheme::NormalBackground).color();
        }
    case Svg::TooltipHover:
        return tooltipColorScheme.decoration(KColorScheme::HoverColor).color();
    case Svg::TooltipFocus:
        return tooltipColorScheme.decoration(KColorScheme::FocusColor).color();
    case Svg::TooltipHighlightedText:
        return selectionColorScheme.foreground(KColorScheme::NormalText).color();
    case Svg::TooltipPositiveText:
        return tooltipColorScheme.foreground(KColorScheme::PositiveText).color();
    case Svg::TooltipNeutralText:
        return tooltipColorScheme.foreground(KColorScheme::NeutralText).color();
    case Svg::TooltipNegativeText:
        return tooltipColorScheme.foreground(KColorScheme::NegativeText).color();

    case Svg::ComplementaryText:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.foreground(KColorScheme::NormalText).color();
        } else {
            return complementaryColorScheme.foreground(KColorScheme::NormalText).color();
        }
    case Svg::ComplementaryBackground:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.background(KColorScheme::NormalBackground).color();
        } else {
            return complementaryColorScheme.background(KColorScheme::NormalBackground).color();
        }
    case Svg::ComplementaryHover:
        return complementaryColorScheme.decoration(KColorScheme::HoverColor).color();
    case Svg::ComplementaryFocus:
        return complementaryColorScheme.decoration(KColorScheme::FocusColor).color();
    case Svg::ComplementaryHighlightedText:
        return selectionColorScheme.foreground(KColorScheme::NormalText).color();
    case Svg::ComplementaryPositiveText:
        return complementaryColorScheme.foreground(KColorScheme::PositiveText).color();
    case Svg::ComplementaryNeutralText:
        return complementaryColorScheme.foreground(KColorScheme::NeutralText).color();
    case Svg::ComplementaryNegativeText:
        return complementaryColorScheme.foreground(KColorScheme::NegativeText).color();

    case Svg::HeaderText:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.foreground(KColorScheme::NormalText).color();
        } else {
            return headerColorScheme.foreground(KColorScheme::NormalText).color();
        }
    case Svg::HeaderBackground:
        if (status == Svg::Status::Selected) {
            return selectionColorScheme.background(KColorScheme::NormalBackground).color();
        } else {
            return headerColorScheme.background(KColorScheme::NormalBackground).color();
        }
    case Svg::HeaderHover:
        return headerColorScheme.decoration(KColorScheme::HoverColor).color();
    case Svg::HeaderFocus:
        return headerColorScheme.decoration(KColorScheme::FocusColor).color();
    case Svg::HeaderHighlightedText:
        return selectionColorScheme.foreground(KColorScheme::NormalText).color();
    case Svg::HeaderPositiveText:
        return headerColorScheme.foreground(KColorScheme::PositiveText).color();
    case Svg::HeaderNeutralText:
        return headerColorScheme.foreground(KColorScheme::NeutralText).color();
    case Svg::HeaderNegativeText:
        return headerColorScheme.foreground(KColorScheme::NegativeText).color();
    case Svg::Frame:
        return KColorUtils::mix(currentScheme->background().color(), currentScheme->foreground().color(), KColorScheme::frameContrast());
    default:
        return {};
    }
}

const QString ImageSetPrivate::svgStyleSheet(KSvg::Svg *svg)
{
    const KSvg::Svg::Status status = svg->status();
    const KColorScheme::ColorSet colorSet = KColorScheme::ColorSet(svg->colorSet());
    const bool useCache = svg->d->colorOverrides.isEmpty();
    QString stylesheet;
    if (useCache) {
        stylesheet = (status == Svg::Status::Selected)
            ? cachedSelectedSvgStyleSheets.value(colorSet)
            : (status == Svg::Status::Inactive ? cachedInactiveSvgStyleSheets.value(colorSet) : cachedSvgStyleSheets.value(colorSet));
    }

    if (stylesheet.isEmpty()) {
        const QList<Svg::StyleSheetColor> namedColors({Svg::Text,
                                                       Svg::Background,
                                                       Svg::Highlight,
                                                       Svg::HighlightedText,
                                                       Svg::PositiveText,
                                                       Svg::NeutralText,
                                                       Svg::NegativeText,

                                                       Svg::ButtonText,
                                                       Svg::ButtonBackground,
                                                       Svg::ButtonHover,
                                                       Svg::ButtonFocus,
                                                       Svg::ButtonHighlightedText,
                                                       Svg::ButtonPositiveText,
                                                       Svg::ButtonNeutralText,
                                                       Svg::ButtonNegativeText,

                                                       Svg::ViewText,
                                                       Svg::ViewBackground,
                                                       Svg::ViewHover,
                                                       Svg::ViewFocus,
                                                       Svg::ViewHighlightedText,
                                                       Svg::ViewPositiveText,
                                                       Svg::ViewNeutralText,
                                                       Svg::ViewNegativeText,

                                                       Svg::TooltipText,
                                                       Svg::TooltipBackground,
                                                       Svg::TooltipHover,
                                                       Svg::TooltipFocus,
                                                       Svg::TooltipHighlightedText,
                                                       Svg::TooltipPositiveText,
                                                       Svg::TooltipNeutralText,
                                                       Svg::TooltipNegativeText,

                                                       Svg::ComplementaryText,
                                                       Svg::ComplementaryBackground,
                                                       Svg::ComplementaryHover,
                                                       Svg::ComplementaryFocus,
                                                       Svg::ComplementaryHighlightedText,
                                                       Svg::ComplementaryPositiveText,
                                                       Svg::ComplementaryNeutralText,
                                                       Svg::ComplementaryNegativeText,

                                                       Svg::HeaderText,
                                                       Svg::HeaderBackground,
                                                       Svg::HeaderHover,
                                                       Svg::HeaderFocus,
                                                       Svg::HeaderHighlightedText,
                                                       Svg::HeaderPositiveText,
                                                       Svg::HeaderNeutralText,
                                                       Svg::HeaderNegativeText,
                                                       Svg::Frame});
        const QString skel = QStringLiteral(".ColorScheme-%1{color:%2;}");
        const QMetaEnum metaEnum = QMetaEnum::fromType<Svg::StyleSheetColor>();

        for (const auto colorName : std::as_const(namedColors)) {
            stylesheet += skel.arg(QString::fromUtf8(metaEnum.valueToKey(colorName)), svg->color(colorName).name());
        }

        if (status == Svg::Status::Selected) {
            cachedSelectedSvgStyleSheets.insert(colorSet, stylesheet);
        } else if (status == Svg::Status::Inactive) {
            cachedInactiveSvgStyleSheets.insert(colorSet, stylesheet);
        } else {
            cachedSvgStyleSheets.insert(colorSet, stylesheet);
        }
    }

    return stylesheet;
}

bool ImageSetPrivate::findInCache(const QString &key, QPixmap &pix)
{
    if (!useCache() || QThread::currentThread() != QCoreApplication::instance()->thread()) {
        return false;
    }
    const QString id = keysToCache.value(key);
    const auto it = pixmapsToCache.constFind(id);
    if (it != pixmapsToCache.constEnd()) {
        pix = *it;
        return !pix.isNull();
    }

    QPixmap temp;
    if (QPixmapCache::find(QLatin1String("ksvg:") + key, &temp) && !temp.isNull()) {
        pix = temp;
        return true;
    }

    return false;
}

void ImageSetPrivate::insertIntoCache(const QString &key, const QPixmap &pix)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, key, pix]() {
                insertIntoCache(key, pix);
            },
            Qt::QueuedConnection);
        return;
    }

    if (useCache()) {
        if (QPixmapCache::insert(QLatin1String("ksvg:") + key, pix)) {
            cachedPixmapKeys.insert(key);
        }
    }
}

void ImageSetPrivate::insertIntoCache(const QString &key, const QPixmap &pix, const QString &id)
{
    if (QThread::currentThread() != thread()) {
        QMetaObject::invokeMethod(
            this,
            [this, key, pix, id]() {
                insertIntoCache(key, pix, id);
            },
            Qt::QueuedConnection);
        return;
    }

    if (useCache()) {
        // Remove old key -> id mapping first
        if (auto key = idsToCache.find(id); key != idsToCache.end()) {
            keysToCache.remove(*key);
        }
        pixmapsToCache[id] = pix;
        keysToCache[key] = id;
        idsToCache[id] = key;

        // Always start the timer in its own thread.
        QMetaObject::invokeMethod(pixmapSaveTimer, "start", Qt::QueuedConnection);
    }
}

void ImageSetPrivate::setImageSetName(const QString &tempImageSetName, bool emitChanged)
{
    QString theme = tempImageSetName;
    if (theme.isEmpty() || theme == imageSetName) {
        // let's try and get the default theme at least
        if (imageSetName.isEmpty()) {
            theme = QLatin1String(ImageSetPrivate::defaultImageSet);
        } else {
            return;
        }
    }

    KPluginMetaData data = metaDataForImageSet(basePath, theme);
    if (!data.isValid()) {
        data = metaDataForImageSet(basePath, QStringLiteral("default"));
        if (!data.isValid()) {
            return;
        }

        theme = QLatin1String(ImageSetPrivate::defaultImageSet);
    }

    // check again as ImageSetPrivate::defaultImageSet might be empty
    if (imageSetName == theme) {
        return;
    }

    imageSetName = theme;
    hasColorsKnown = false;

    // load the color scheme config
    const QString colorsFile = QStandardPaths::locate(QStandardPaths::GenericDataLocation, basePath % theme % QLatin1String("/colors"));
    // qCDebug(LOG_KSVG) << "we're going for..." << colorsFile << "*******************";

    if (colorsFile.isEmpty()) {
        colors = nullptr;
    } else {
        colors = KSharedConfig::openConfig(colorsFile);
    }

    colorScheme = KColorScheme(QPalette::Active, KColorScheme::Window, colors);
    selectionColorScheme = KColorScheme(QPalette::Active, KColorScheme::Selection, colors);
    buttonColorScheme = KColorScheme(QPalette::Active, KColorScheme::Button, colors);
    viewColorScheme = KColorScheme(QPalette::Active, KColorScheme::View, colors);
    complementaryColorScheme = KColorScheme(QPalette::Active, KColorScheme::Complementary, colors);
    headerColorScheme = KColorScheme(QPalette::Active, KColorScheme::Header, colors);
    tooltipColorScheme = KColorScheme(QPalette::Active, KColorScheme::Tooltip, colors);

    fallbackImageSets = {QLatin1String(ImageSetPrivate::defaultImageSet)};

#if KSVG_BUILD_DEPRECATED_SINCE(6, 21)
    pluginMetaData = metaDataForImageSet(basePath, theme);
#endif

    if (emitChanged) {
        scheduleImageSetChangeNotification(PixmapCache | SvgElementsCache);
    }
}

bool ImageSetPrivate::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == QCoreApplication::instance()) {
        if (event->type() == QEvent::ApplicationPaletteChange) {
            colorsChanged();
        }
    }
    return QObject::eventFilter(watched, event);
}
}

#include "moc_imageset_p.cpp"
