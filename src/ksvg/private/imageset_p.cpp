/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "imageset_p.h"
#include "debug_p.h"
#include "framesvg.h"
#include "framesvg_p.h"
#include "imageset.h"
#include "svg_p.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>

#include <KDirWatch>
#include <KSharedConfig>
#include <kpluginmetadata.h>

#define DEFAULT_CACHE_SIZE 16384 // value is from the old kconfigxt default value

namespace KSvg
{
const char ImageSetPrivate::defaultImageSet[] = "default";
// the system colors theme is used to cache unthemed svgs with colorization needs
// these svgs do not follow the theme's colors, but rather the system colors
const char ImageSetPrivate::systemColorsImageSet[] = "internal-system-colors";

ImageSetPrivate *ImageSetPrivate::globalImageSet = nullptr;
QHash<QString, ImageSetPrivate *> ImageSetPrivate::themes = QHash<QString, ImageSetPrivate *>();
using QSP = QStandardPaths;

KSharedConfig::Ptr configForImageSet(const QString &basePath, const QString &theme)
{
    const QString baseName = basePath % theme;
    QString configPath = QSP::locate(QSP::GenericDataLocation, baseName + QLatin1String("/config"));
    if (!configPath.isEmpty()) {
        return KSharedConfig::openConfig(configPath, KConfig::SimpleConfig);
    }
    QString metadataPath = QSP::locate(QSP::GenericDataLocation, baseName + QLatin1String("/metadata.desktop"));
    return KSharedConfig::openConfig(metadataPath, KConfig::SimpleConfig);
}

KPluginMetaData metaDataForImageSet(const QString &basePath, const QString &theme)
{
    const QString packageBasePath = QSP::locate(QSP::GenericDataLocation, basePath % theme, QSP::LocateDirectory);
    if (packageBasePath.isEmpty()) {
        qWarning(LOG_KSVG) << "Could not locate plasma theme" << theme << "in" << basePath << "using search path"
                           << QSP::standardLocations(QSP::GenericDataLocation);
        return {};
    }
    if (QFileInfo::exists(packageBasePath + QLatin1String("/metadata.json"))) {
        return KPluginMetaData::fromJsonFile(packageBasePath + QLatin1String("/metadata.json"));
    } else {
        qCWarning(LOG_KSVG) << "Could not locate metadata for theme" << theme;
        return {};
    }
}

ImageSetPrivate::ImageSetPrivate(QObject *parent)
    : QObject(parent)
    , colorScheme(QPalette::Active, KColorScheme::Window, KSharedConfigPtr(nullptr))
    , selectionColorScheme(QPalette::Active, KColorScheme::Selection, KSharedConfigPtr(nullptr))
    , buttonColorScheme(QPalette::Active, KColorScheme::Button, KSharedConfigPtr(nullptr))
    , viewColorScheme(QPalette::Active, KColorScheme::View, KSharedConfigPtr(nullptr))
    , complementaryColorScheme(QPalette::Active, KColorScheme::Complementary, KSharedConfigPtr(nullptr))
    , headerColorScheme(QPalette::Active, KColorScheme::Header, KSharedConfigPtr(nullptr))
    , tooltipColorScheme(QPalette::Active, KColorScheme::Tooltip, KSharedConfigPtr(nullptr))
    , pixmapCache(nullptr)
    , cacheSize(DEFAULT_CACHE_SIZE)
    , cachesToDiscard(NoCache)
    , isDefault(true)
    , useGlobal(true)
    , cacheImageSet(true)
    , fixedName(false)
    , apiMajor(1)
    , apiMinor(0)
    , apiRevision(0)
{
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
    delete pixmapCache;
}

bool ImageSetPrivate::useCache()
{
    bool cachesTooOld = false;

    if (cacheImageSet && !pixmapCache) {
        if (cacheSize == 0) {
            cacheSize = DEFAULT_CACHE_SIZE;
        }
        const bool isRegularImageSet = imageSetName != QLatin1String(systemColorsImageSet);
        QString cacheFile = QLatin1String("plasma_theme_") + imageSetName;

        // clear any cached values from the previous theme cache
        themeVersion.clear();

        if (!themeMetadataPath.isEmpty()) {
            KDirWatch::self()->removeFile(themeMetadataPath);
        }
        themeMetadataPath = configForImageSet(basePath, imageSetName)->name();
        if (isRegularImageSet) {
            const QString cacheFileBase = cacheFile + QLatin1String("*.kcache");

            QString currentCacheFileName;
            if (!themeMetadataPath.isEmpty()) {
                // now we record the theme version, if we can
                const KPluginMetaData data = metaDataForImageSet(basePath, imageSetName);
                if (data.isValid()) {
                    themeVersion = data.version();
                }
                if (!themeVersion.isEmpty()) {
                    cacheFile += QLatin1String("_v") + themeVersion;
                    currentCacheFileName = cacheFile + QLatin1String(".kcache");
                }
            }

            // now we check for, and remove if necessary, old caches
            QDir cacheDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation));
            cacheDir.setNameFilters(QStringList({cacheFileBase}));

            const auto files = cacheDir.entryInfoList();
            for (const QFileInfo &file : files) {
                if (currentCacheFileName.isEmpty() //
                    || !file.absoluteFilePath().endsWith(currentCacheFileName)) {
                    QFile::remove(file.absoluteFilePath());
                }
            }
        }

        // now we do a sanity check: if the metadata.desktop file is newer than the cache, drop the cache
        if (isRegularImageSet && !themeMetadataPath.isEmpty()) {
            // now we check to see if the theme metadata file itself is newer than the pixmap cache
            // this is done before creating the pixmapCache object since that can change the mtime
            // on the cache file

            // FIXME: when using the system colors, if they change while the application is not running
            // the cache should be dropped; we need a way to detect system color change when the
            // application is not running.
            // check for expired cache
            const QString cacheFilePath =
                QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1Char('/') + cacheFile + QLatin1String(".kcache");
            if (!cacheFilePath.isEmpty()) {
                const QFileInfo cacheFileInfo(cacheFilePath);
                const QFileInfo metadataFileInfo(themeMetadataPath);
                const QFileInfo iconImageSetMetadataFileInfo(iconImageSetMetadataPath);

                cachesTooOld = (cacheFileInfo.lastModified().toSecsSinceEpoch() < metadataFileInfo.lastModified().toSecsSinceEpoch())
                    || (cacheFileInfo.lastModified().toSecsSinceEpoch() < iconImageSetMetadataFileInfo.lastModified().toSecsSinceEpoch());
            }
        }

        pixmapCache = new KImageCache(cacheFile, cacheSize * 1024);
        pixmapCache->setEvictionPolicy(KSharedDataCache::EvictLeastRecentlyUsed);

        if (cachesTooOld) {
            discardCache(PixmapCache | SvgElementsCache);
        }
    }

    return cacheImageSet;
}

void ImageSetPrivate::onAppExitCleanup()
{
    pixmapsToCache.clear();
    delete pixmapCache;
    pixmapCache = nullptr;
    cacheImageSet = false;
}

QString ImageSetPrivate::imagePath(const QString &theme, const QString &type, const QString &image)
{
    QString subdir = basePath % theme % type % image;
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, subdir);
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
        if (pixmapCache) {
            pixmapCache->clear();
        }
    } else {
        // This deletes the object but keeps the on-disk cache for later use
        delete pixmapCache;
        pixmapCache = nullptr;
    }

    cachedSvgStyleSheets.clear();
    cachedSelectedSvgStyleSheets.clear();
    cachedInactiveSvgStyleSheets.clear();

    if (caches & SvgElementsCache) {
        discoveries.clear();
    }
}

void ImageSetPrivate::scheduledCacheUpdate()
{
    if (useCache()) {
        QHashIterator<QString, QPixmap> it(pixmapsToCache);
        while (it.hasNext()) {
            it.next();
            pixmapCache->insertPixmap(idsToCache[it.key()], it.value());
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
    Q_EMIT applicationPaletteChange();
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
    Q_EMIT imageSetChanged();
}

const QString ImageSetPrivate::processStyleSheet(const QString &css, KSvg::Svg::Status status)
{
    QString stylesheet(css);
    QHash<QString, QString> elements;
    // If you add elements here, make sure their names are sufficiently unique to not cause
    // clashes between element keys
    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%textcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    } else if (status == Svg::Status::Inactive) {
        elements[QStringLiteral("%textcolor")] = colorScheme.foreground(KColorScheme::InactiveText).color().name();
    } else {
        elements[QStringLiteral("%textcolor")] = colorScheme.foreground(KColorScheme::NormalText).color().name();
    }

    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%backgroundcolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    } else {
        elements[QStringLiteral("%backgroundcolor")] = colorScheme.background(KColorScheme::NormalBackground).color().name();
    }

    elements[QStringLiteral("%highlightcolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    elements[QStringLiteral("%highlightedtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    elements[QStringLiteral("%visitedlink")] = colorScheme.foreground(KColorScheme::VisitedText).color().name();
    elements[QStringLiteral("%activatedlink")] = colorScheme.foreground(KColorScheme::ActiveText).color().name();
    elements[QStringLiteral("%hoveredlink")] = colorScheme.foreground(KColorScheme::ActiveText).color().name();
    elements[QStringLiteral("%link")] = colorScheme.foreground(KColorScheme::LinkText).color().name();
    elements[QStringLiteral("%positivetextcolor")] = colorScheme.foreground(KColorScheme::PositiveText).color().name();
    elements[QStringLiteral("%neutraltextcolor")] = colorScheme.foreground(KColorScheme::NeutralText).color().name();
    elements[QStringLiteral("%negativetextcolor")] = colorScheme.foreground(KColorScheme::NegativeText).color().name();

    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%buttontextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    } else {
        elements[QStringLiteral("%buttontextcolor")] = buttonColorScheme.foreground(KColorScheme::NormalText).color().name();
    }
    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%buttontextcolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    } else {
        elements[QStringLiteral("%buttontextcolor")] = buttonColorScheme.background(KColorScheme::NormalBackground).color().name();
    }

    elements[QStringLiteral("%buttonhovercolor")] = buttonColorScheme.decoration(KColorScheme::HoverColor).color().name();
    elements[QStringLiteral("%buttonfocuscolor")] = buttonColorScheme.decoration(KColorScheme::FocusColor).color().name();
    elements[QStringLiteral("%buttonhighlightedtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    elements[QStringLiteral("%buttonpositivetextcolor")] = buttonColorScheme.foreground(KColorScheme::PositiveText).color().name();
    elements[QStringLiteral("%buttonneutraltextcolor")] = buttonColorScheme.foreground(KColorScheme::NeutralText).color().name();
    elements[QStringLiteral("%buttonnegativetextcolor")] = buttonColorScheme.foreground(KColorScheme::NegativeText).color().name();

    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%viewtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    } else {
        elements[QStringLiteral("%viewtextcolor")] = viewColorScheme.foreground(KColorScheme::NormalText).color().name();
    }
    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%viewhovercolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    } else {
        elements[QStringLiteral("%viewhovercolor")] = viewColorScheme.background(KColorScheme::NormalBackground).color().name();
    }

    elements[QStringLiteral("%viewhovercolor")] = buttonColorScheme.decoration(KColorScheme::HoverColor).color().name();
    elements[QStringLiteral("%viewfocuscolor")] = buttonColorScheme.decoration(KColorScheme::FocusColor).color().name();
    elements[QStringLiteral("%viewhighlightedtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    elements[QStringLiteral("%viewpositivetextcolor")] = buttonColorScheme.foreground(KColorScheme::PositiveText).color().name();
    elements[QStringLiteral("%viewneutraltextcolor")] = buttonColorScheme.foreground(KColorScheme::NeutralText).color().name();
    elements[QStringLiteral("%viewnegativetextcolor")] = buttonColorScheme.foreground(KColorScheme::NegativeText).color().name();

    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%tooltiptextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    } else {
        elements[QStringLiteral("%tooltiptextcolor")] = tooltipColorScheme.foreground(KColorScheme::NormalText).color().name();
    }
    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%tooltipbackgroundcolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    } else {
        elements[QStringLiteral("%tooltipbackgroundcolor")] = tooltipColorScheme.background(KColorScheme::NormalBackground).color().name();
    }

    elements[QStringLiteral("%tooltiphovercolor")] = buttonColorScheme.decoration(KColorScheme::HoverColor).color().name();
    elements[QStringLiteral("%tooltipfocuscolor")] = buttonColorScheme.decoration(KColorScheme::FocusColor).color().name();
    elements[QStringLiteral("%tooltiphighlightedtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    elements[QStringLiteral("%tooltippositivetextcolor")] = buttonColorScheme.foreground(KColorScheme::PositiveText).color().name();
    elements[QStringLiteral("%tooltipneutraltextcolor")] = buttonColorScheme.foreground(KColorScheme::NeutralText).color().name();
    elements[QStringLiteral("%tooltipnegativetextcolor")] = buttonColorScheme.foreground(KColorScheme::NegativeText).color().name();

    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%complementarytextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    } else {
        elements[QStringLiteral("%complementarytextcolor")] = tooltipColorScheme.foreground(KColorScheme::NormalText).color().name();
    }
    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%complementarybackgroundcolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    } else {
        elements[QStringLiteral("%complementarybackgroundcolor")] = tooltipColorScheme.background(KColorScheme::NormalBackground).color().name();
    }

    elements[QStringLiteral("%complementaryhovercolor")] = buttonColorScheme.decoration(KColorScheme::HoverColor).color().name();
    elements[QStringLiteral("%complementaryfocuscolor")] = buttonColorScheme.decoration(KColorScheme::FocusColor).color().name();
    elements[QStringLiteral("%complementaryhighlightedtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    elements[QStringLiteral("%complementarypositivetextcolor")] = buttonColorScheme.foreground(KColorScheme::PositiveText).color().name();
    elements[QStringLiteral("%complementaryneutraltextcolor")] = buttonColorScheme.foreground(KColorScheme::NeutralText).color().name();
    elements[QStringLiteral("%complementarynegativetextcolor")] = buttonColorScheme.foreground(KColorScheme::NegativeText).color().name();

    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%headertextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    } else {
        elements[QStringLiteral("%headertextcolor")] = tooltipColorScheme.foreground(KColorScheme::NormalText).color().name();
    }
    if (status == Svg::Status::Selected) {
        elements[QStringLiteral("%headerbackgroundcolor")] = selectionColorScheme.background(KColorScheme::NormalBackground).color().name();
    } else {
        elements[QStringLiteral("%headerbackgroundcolor")] = tooltipColorScheme.background(KColorScheme::NormalBackground).color().name();
    }

    elements[QStringLiteral("%headerhovercolor")] = buttonColorScheme.decoration(KColorScheme::HoverColor).color().name();
    elements[QStringLiteral("%headerfocuscolor")] = buttonColorScheme.decoration(KColorScheme::FocusColor).color().name();
    elements[QStringLiteral("%headerhighlightedtextcolor")] = selectionColorScheme.foreground(KColorScheme::NormalText).color().name();
    elements[QStringLiteral("%headerpositivetextcolor")] = buttonColorScheme.foreground(KColorScheme::PositiveText).color().name();
    elements[QStringLiteral("%headerneutraltextcolor")] = buttonColorScheme.foreground(KColorScheme::NeutralText).color().name();
    elements[QStringLiteral("%headernegativetextcolor")] = buttonColorScheme.foreground(KColorScheme::NegativeText).color().name();

    QFont font = QGuiApplication::font();
    elements[QStringLiteral("%fontsize")] = QStringLiteral("%1pt").arg(font.pointSize());
    QString family{font.family()};
    family.truncate(family.indexOf(QLatin1Char('[')));
    elements[QStringLiteral("%fontfamily")] = family;
    elements[QStringLiteral("%smallfontsize")] = QStringLiteral("%1pt").arg(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont).pointSize());

    QHash<QString, QString>::const_iterator it = elements.constBegin();
    QHash<QString, QString>::const_iterator itEnd = elements.constEnd();
    for (; it != itEnd; ++it) {
        stylesheet.replace(it.key(), it.value());
    }
    return stylesheet;
}

const QString ImageSetPrivate::svgStyleSheet(KColorScheme::ColorSet group, KSvg::Svg::Status status)
{
    QString stylesheet = (status == Svg::Status::Selected)
        ? cachedSelectedSvgStyleSheets.value(group)
        : (status == Svg::Status::Inactive ? cachedInactiveSvgStyleSheets.value(group) : cachedSvgStyleSheets.value(group));
    if (stylesheet.isEmpty()) {
        QString skel = QStringLiteral(".ColorScheme-%1{color:%2;}");

        switch (group) {
        case KColorScheme::Button:
            stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%buttontextcolor"));
            stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%buttonbackgroundcolor"));

            stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%buttonhovercolor"));
            stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%buttonhighlightedtextcolor"));
            stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%buttonpositivetextcolor"));
            stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%buttonneutraltextcolor"));
            stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%buttonnegativetextcolor"));
            break;
        case KColorScheme::View:
            stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%viewtextcolor"));
            stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%viewbackgroundcolor"));

            stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%viewhovercolor"));
            stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%viewhighlightedtextcolor"));
            stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%viewpositivetextcolor"));
            stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%viewneutraltextcolor"));
            stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%viewnegativetextcolor"));
            break;
        case KColorScheme::Complementary:
            stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%complementarytextcolor"));
            stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%complementarybackgroundcolor"));

            stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%complementaryhovercolor"));
            stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%complementaryhighlightedtextcolor"));
            stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%complementarypositivetextcolor"));
            stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%complementaryneutraltextcolor"));
            stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%complementarynegativetextcolor"));
            break;
        case KColorScheme::Header:
            stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%headertextcolor"));
            stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%headerbackgroundcolor"));

            stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%headerhovercolor"));
            stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%headerhighlightedtextcolor"));
            stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%headerpositivetextcolor"));
            stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%headerneutraltextcolor"));
            stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%headernegativetextcolor"));
            break;
        case KColorScheme::Tooltip:
            stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%tooltiptextcolor"));
            stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%tooltipbackgroundcolor"));

            stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%tooltiphovercolor"));
            stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%tooltiphighlightedtextcolor"));
            stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%tooltippositivetextcolor"));
            stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%tooltipneutraltextcolor"));
            stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%tooltipnegativetextcolor"));
            break;
        default:
            stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%textcolor"));
            stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%backgroundcolor"));

            stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%highlightcolor"));
            stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%highlightedtextcolor"));
            stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%positivetextcolor"));
            stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%neutraltextcolor"));
            stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%negativetextcolor"));
        }

        stylesheet += skel.arg(QStringLiteral("ButtonText"), QStringLiteral("%buttontextcolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonBackground"), QStringLiteral("%buttonbackgroundcolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonHover"), QStringLiteral("%buttonhovercolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonFocus"), QStringLiteral("%buttonfocuscolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonHighlightedText"), QStringLiteral("%buttonhighlightedtextcolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonPositiveText"), QStringLiteral("%buttonpositivetextcolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonNeutralText"), QStringLiteral("%buttonneutraltextcolor"));
        stylesheet += skel.arg(QStringLiteral("ButtonNegativeText"), QStringLiteral("%buttonnegativetextcolor"));

        stylesheet += skel.arg(QStringLiteral("ViewText"), QStringLiteral("%viewtextcolor"));
        stylesheet += skel.arg(QStringLiteral("ViewBackground"), QStringLiteral("%viewbackgroundcolor"));
        stylesheet += skel.arg(QStringLiteral("ViewHover"), QStringLiteral("%viewhovercolor"));
        stylesheet += skel.arg(QStringLiteral("ViewFocus"), QStringLiteral("%viewfocuscolor"));
        stylesheet += skel.arg(QStringLiteral("ViewHighlightedText"), QStringLiteral("%viewhighlightedtextcolor"));
        stylesheet += skel.arg(QStringLiteral("ViewPositiveText"), QStringLiteral("%viewpositivetextcolor"));
        stylesheet += skel.arg(QStringLiteral("ViewNeutralText"), QStringLiteral("%viewneutraltextcolor"));
        stylesheet += skel.arg(QStringLiteral("ViewNegativeText"), QStringLiteral("%viewnegativetextcolor"));

        stylesheet += skel.arg(QStringLiteral("ComplementaryText"), QStringLiteral("%complementarytextcolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryBackground"), QStringLiteral("%complementarybackgroundcolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryHover"), QStringLiteral("%complementaryhovercolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryFocus"), QStringLiteral("%complementaryfocuscolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryHighlightedText"), QStringLiteral("%complementaryhighlightedtextcolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryPositiveText"), QStringLiteral("%complementarypositivetextcolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryNeutralText"), QStringLiteral("%complementaryneutraltextcolor"));
        stylesheet += skel.arg(QStringLiteral("ComplementaryNegativeText"), QStringLiteral("%complementarynegativetextcolor"));

        stylesheet += skel.arg(QStringLiteral("HeaderText"), QStringLiteral("%headertextcolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderBackground"), QStringLiteral("%headerbackgroundcolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderHover"), QStringLiteral("%headerhovercolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderFocus"), QStringLiteral("%headerfocuscolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderHighlightedText"), QStringLiteral("%headerhighlightedtextcolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderPositiveText"), QStringLiteral("%headerpositivetextcolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderNeutralText"), QStringLiteral("%headerneutraltextcolor"));
        stylesheet += skel.arg(QStringLiteral("HeaderNegativeText"), QStringLiteral("%headernegativetextcolor"));

        stylesheet += skel.arg(QStringLiteral("TootipText"), QStringLiteral("%tooltiptextcolor"));
        stylesheet += skel.arg(QStringLiteral("TootipBackground"), QStringLiteral("%tooltipbackgroundcolor"));
        stylesheet += skel.arg(QStringLiteral("TootipHover"), QStringLiteral("%tooltiphovercolor"));
        stylesheet += skel.arg(QStringLiteral("TootipFocus"), QStringLiteral("%tooltipfocuscolor"));
        stylesheet += skel.arg(QStringLiteral("TootipHighlightedText"), QStringLiteral("%tooltiphighlightedtextcolor"));
        stylesheet += skel.arg(QStringLiteral("TootipPositiveText"), QStringLiteral("%tooltippositivetextcolor"));
        stylesheet += skel.arg(QStringLiteral("TootipNeutralText"), QStringLiteral("%tooltipneutraltextcolor"));
        stylesheet += skel.arg(QStringLiteral("TootipNegativeText"), QStringLiteral("%tooltipnegativetextcolor"));

        stylesheet = processStyleSheet(stylesheet, status);
        if (status == Svg::Status::Selected) {
            cachedSelectedSvgStyleSheets.insert(group, stylesheet);
        } else if (status == Svg::Status::Inactive) {
            cachedInactiveSvgStyleSheets.insert(group, stylesheet);
        } else {
            cachedSvgStyleSheets.insert(group, stylesheet);
        }
    }

    return stylesheet;
}

bool ImageSetPrivate::findInCache(const QString &key, QPixmap &pix, unsigned int lastModified)
{
    if (lastModified == 0) {
        qCWarning(LOG_KSVG) << "findInCache with a lastModified timestamp of 0 is deprecated";
        return false;
    }

    if (!useCache()) {
        return false;
    }

    if (lastModified > uint(pixmapCache->lastModifiedTime().toSecsSinceEpoch())) {
        return false;
    }

    const QString id = keysToCache.value(key);
    const auto it = pixmapsToCache.constFind(id);
    if (it != pixmapsToCache.constEnd()) {
        pix = *it;
        return !pix.isNull();
    }

    QPixmap temp;
    if (pixmapCache->findPixmap(key, &temp) && !temp.isNull()) {
        pix = temp;
        return true;
    }

    return false;
}

void ImageSetPrivate::insertIntoCache(const QString &key, const QPixmap &pix)
{
    if (useCache()) {
        pixmapCache->insertPixmap(key, pix);
    }
}

void ImageSetPrivate::insertIntoCache(const QString &key, const QPixmap &pix, const QString &id)
{
    if (useCache()) {
        pixmapsToCache[id] = pix;
        keysToCache[key] = id;
        idsToCache[id] = key;

        // always start timer in pixmapSaveTimer's thread
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

    // we have one special theme: essentially a dummy theme used to cache things with
    // the system colors.
    bool realImageSet = theme != QLatin1String(systemColorsImageSet);
    if (realImageSet) {
        KPluginMetaData data = metaDataForImageSet(basePath, theme);
        if (!data.isValid()) {
            data = metaDataForImageSet(basePath, QStringLiteral("default"));
            if (!data.isValid()) {
                return;
            }

            theme = QLatin1String(ImageSetPrivate::defaultImageSet);
        }
    }

    // check again as ImageSetPrivate::defaultImageSet might be empty
    if (imageSetName == theme) {
        return;
    }

    imageSetName = theme;

    // load the color scheme config
    const QString colorsFile =
        realImageSet ? QStandardPaths::locate(QStandardPaths::GenericDataLocation, basePath % theme % QLatin1String("/colors")) : QString();

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

    if (realImageSet) {
        pluginMetaData = metaDataForImageSet(basePath, theme);
        KSharedConfigPtr metadata = configForImageSet(basePath, theme);

        KConfigGroup cg(metadata, "Settings");
        QString fallback = cg.readEntry("FallbackImageSet", QString());

        fallbackImageSets.clear();
        while (!fallback.isEmpty() && !fallbackImageSets.contains(fallback)) {
            fallbackImageSets.append(fallback);

            KSharedConfigPtr metadata = configForImageSet(basePath, fallback);
            KConfigGroup cg(metadata, "Settings");
            fallback = cg.readEntry("FallbackImageSet", QString());
        }

        if (!fallbackImageSets.contains(QLatin1String(ImageSetPrivate::defaultImageSet))) {
            fallbackImageSets.append(QLatin1String(ImageSetPrivate::defaultImageSet));
        }

        // Check for what Plasma version the theme has been done
        // There are some behavioral differences between KDE4 Plasma and Plasma 5
        const QString apiVersion = pluginMetaData.value(QStringLiteral("X-Plasma-API"));
        apiMajor = 1;
        apiMinor = 0;
        apiRevision = 0;
        if (!apiVersion.isEmpty()) {
            const QVector<QStringView> parts = QStringView(apiVersion).split(QLatin1Char('.'));
            if (!parts.isEmpty()) {
                apiMajor = parts.value(0).toInt();
            }
            if (parts.count() > 1) {
                apiMinor = parts.value(1).toInt();
            }
            if (parts.count() > 2) {
                apiRevision = parts.value(2).toInt();
            }
        }
    }

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
