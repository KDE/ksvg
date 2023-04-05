/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "theme_p.h"
#include "debug_p.h"
#include "framesvg.h"
#include "framesvg_p.h"
#include "svg_p.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>

#include <KDirWatch>
#include <KIconLoader>
#include <KIconTheme>
#include <KSharedConfig>
#include <Kirigami/KirigamiPluginFactory>
#include <kpluginmetadata.h>

#define DEFAULT_CACHE_SIZE 16384 // value is from the old kconfigxt default value

namespace KSvg
{
const char ThemePrivate::defaultTheme[] = "default";
const char ThemePrivate::themeRcFile[] = "plasmarc";
// the system colors theme is used to cache unthemed svgs with colorization needs
// these svgs do not follow the theme's colors, but rather the system colors
const char ThemePrivate::systemColorsTheme[] = "internal-system-colors";

ThemePrivate *ThemePrivate::globalTheme = nullptr;
QHash<QString, ThemePrivate *> ThemePrivate::themes = QHash<QString, ThemePrivate *>();
using QSP = QStandardPaths;

KSharedConfig::Ptr configForTheme(const QString &basePath, const QString &theme)
{
    const QString baseName = basePath % theme;
    QString configPath = QSP::locate(QSP::GenericDataLocation, baseName + QLatin1String("/plasmarc"));
    if (!configPath.isEmpty()) {
        return KSharedConfig::openConfig(configPath, KConfig::SimpleConfig);
    }
    QString metadataPath = QSP::locate(QSP::GenericDataLocation, baseName + QLatin1String("/metadata.desktop"));
    return KSharedConfig::openConfig(metadataPath, KConfig::SimpleConfig);
}

KPluginMetaData metaDataForTheme(const QString &basePath, const QString &theme)
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

ThemePrivate::ThemePrivate(QObject *parent)
    : QObject(parent)
    , pixmapCache(nullptr)
    , cacheSize(DEFAULT_CACHE_SIZE)
    , cachesToDiscard(NoCache)
    , isDefault(true)
    , useGlobal(true)
    , cacheTheme(true)
    , fixedName(false)
    , apiMajor(1)
    , apiMinor(0)
    , apiRevision(0)
{
    auto plugin = Kirigami::KirigamiPluginFactory::findPlugin();
    if (plugin) {
        kirigamiTheme = plugin->createPlatformTheme(this);
    }

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
    QObject::connect(pixmapSaveTimer, &QTimer::timeout, this, &ThemePrivate::scheduledCacheUpdate);

    updateNotificationTimer = new QTimer(this);
    updateNotificationTimer->setSingleShot(true);
    updateNotificationTimer->setInterval(100);
    QObject::connect(updateNotificationTimer, &QTimer::timeout, this, &ThemePrivate::notifyOfChanged);

    QCoreApplication::instance()->installEventFilter(this);

    const QString configFile = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + QLatin1Char('/') + QLatin1String(themeRcFile);
    KDirWatch::self()->addFile(configFile);

    // Catch both, direct changes to the config file ...
    connect(KDirWatch::self(), &KDirWatch::dirty, this, &ThemePrivate::settingsFileChanged);
    // ... but also remove/recreate cycles, like KConfig does it
    connect(KDirWatch::self(), &KDirWatch::created, this, &ThemePrivate::settingsFileChanged);

    QObject::connect(KIconLoader::global(), &KIconLoader::iconChanged, this, [this]() {
        scheduleThemeChangeNotification(PixmapCache | SvgElementsCache);
    });
}

ThemePrivate::~ThemePrivate()
{
    FrameSvgPrivate::s_sharedFrames.remove(this);
    delete pixmapCache;
}

KConfigGroup &ThemePrivate::config()
{
    if (!cfg.isValid()) {
        QString groupName = QStringLiteral("Theme");

        if (!useGlobal) {
            QString app = QCoreApplication::applicationName();

            if (!app.isEmpty()) {
#ifndef NDEBUG
                // qCDebug(LOG_KSVG) << "using theme for app" << app;
#endif
                groupName.append(QLatin1Char('-')).append(app);
            }
        }
        cfg = KConfigGroup(KSharedConfig::openConfig(QFile::decodeName(themeRcFile)), groupName);
    }

    return cfg;
}

bool ThemePrivate::useCache()
{
    bool cachesTooOld = false;

    if (cacheTheme && !pixmapCache) {
        if (cacheSize == 0) {
            cacheSize = DEFAULT_CACHE_SIZE;
        }
        const bool isRegularTheme = themeName != QLatin1String(systemColorsTheme);
        QString cacheFile = QLatin1String("plasma_theme_") + themeName;

        // clear any cached values from the previous theme cache
        themeVersion.clear();

        if (!themeMetadataPath.isEmpty()) {
            KDirWatch::self()->removeFile(themeMetadataPath);
        }
        themeMetadataPath = configForTheme(basePath, themeName)->name();
        if (isRegularTheme) {
            const auto *iconTheme = KIconLoader::global()->theme();
            if (iconTheme) {
                iconThemeMetadataPath = iconTheme->dir() + QStringLiteral("index.theme");
            }

            const QString cacheFileBase = cacheFile + QLatin1String("*.kcache");

            QString currentCacheFileName;
            if (!themeMetadataPath.isEmpty()) {
                // now we record the theme version, if we can
                const KPluginMetaData data = metaDataForTheme(basePath, themeName);
                if (data.isValid()) {
                    themeVersion = data.version();
                }
                if (!themeVersion.isEmpty()) {
                    cacheFile += QLatin1String("_v") + themeVersion;
                    currentCacheFileName = cacheFile + QLatin1String(".kcache");
                }

                // watch the metadata file for changes at runtime
                KDirWatch::self()->addFile(themeMetadataPath);
                QObject::connect(KDirWatch::self(), &KDirWatch::created, this, &ThemePrivate::settingsFileChanged, Qt::UniqueConnection);
                QObject::connect(KDirWatch::self(), &KDirWatch::dirty, this, &ThemePrivate::settingsFileChanged, Qt::UniqueConnection);

                if (!iconThemeMetadataPath.isEmpty()) {
                    KDirWatch::self()->addFile(iconThemeMetadataPath);
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
        if (isRegularTheme && !themeMetadataPath.isEmpty()) {
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
                const QFileInfo iconThemeMetadataFileInfo(iconThemeMetadataPath);

                cachesTooOld = (cacheFileInfo.lastModified().toSecsSinceEpoch() < metadataFileInfo.lastModified().toSecsSinceEpoch())
                    || (cacheFileInfo.lastModified().toSecsSinceEpoch() < iconThemeMetadataFileInfo.lastModified().toSecsSinceEpoch());
            }
        }

        pixmapCache = new KImageCache(cacheFile, cacheSize * 1024);
        pixmapCache->setEvictionPolicy(KSharedDataCache::EvictLeastRecentlyUsed);

        if (cachesTooOld) {
            discardCache(PixmapCache | SvgElementsCache);
        }
    }

    if (cacheTheme) {
        QString currentIconThemePath;
        const auto *iconTheme = KIconLoader::global()->theme();
        if (iconTheme) {
            currentIconThemePath = iconTheme->dir();
        }

        const QString oldIconThemePath = SvgRectsCache::instance()->iconThemePath();
        if (oldIconThemePath != currentIconThemePath) {
            discardCache(PixmapCache | SvgElementsCache);
            SvgRectsCache::instance()->setIconThemePath(currentIconThemePath);
        }
    }

    return cacheTheme;
}

void ThemePrivate::onAppExitCleanup()
{
    pixmapsToCache.clear();
    delete pixmapCache;
    pixmapCache = nullptr;
    cacheTheme = false;
}

QString ThemePrivate::imagePath(const QString &theme, const QString &type, const QString &image)
{
    QString subdir = basePath % theme % type % image;
    return QStandardPaths::locate(QStandardPaths::GenericDataLocation, subdir);
}

QString ThemePrivate::findInTheme(const QString &image, const QString &theme, bool cache)
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

void ThemePrivate::discardCache(CacheTypes caches)
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

void ThemePrivate::scheduledCacheUpdate()
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

void ThemePrivate::scheduleThemeChangeNotification(CacheTypes caches)
{
    cachesToDiscard |= caches;
    updateNotificationTimer->start();
}

void ThemePrivate::notifyOfChanged()
{
    // qCDebug(LOG_KSVG) << cachesToDiscard;
    discardCache(cachesToDiscard);
    cachesToDiscard = NoCache;
    Q_EMIT themeChanged();
}

const QString ThemePrivate::processStyleSheet(const QString &css, KSvg::Svg::Status status, Kirigami::PlatformTheme *theme)
{
    QString stylesheet(css);

    QHash<QString, QString> elements;
    // If you add elements here, make sure their names are sufficiently unique to not cause
    // clashes between element keys
    switch (status) {
    case Svg::Status::Inactive:
        elements[QStringLiteral("%textcolor")] = theme->disabledTextColor().name();
        break;
    case Svg::Status::Selected:
        elements[QStringLiteral("%textcolor")] = theme->highlightedTextColor().name();
        break;
    default:
        elements[QStringLiteral("%textcolor")] = theme->textColor().name();
    }

    switch (status) {
    case Svg::Status::Selected:
        elements[QStringLiteral("%backgroundcolor")] = theme->highlightColor().name();
        break;
    default:
        elements[QStringLiteral("%backgroundcolor")] = theme->backgroundColor().name();
    }

    elements[QStringLiteral("%highlightcolor")] = theme->highlightColor().name();
    elements[QStringLiteral("%highlightedtextcolor")] = theme->highlightedTextColor().name();
    elements[QStringLiteral("%visitedlink")] = theme->visitedLinkColor().name();
    elements[QStringLiteral("%activatedlink")] = theme->highlightColor().name();
    elements[QStringLiteral("%hoveredlink")] = theme->highlightColor().name();
    elements[QStringLiteral("%link")] = theme->linkColor().name();
    elements[QStringLiteral("%positivetextcolor")] = theme->positiveTextColor().name();
    elements[QStringLiteral("%neutraltextcolor")] = theme->neutralTextColor().name();
    elements[QStringLiteral("%negativetextcolor")] = theme->neutralTextColor().name();

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

const QString ThemePrivate::svgStyleSheet(Kirigami::PlatformTheme *theme, KSvg::Svg::Status status)
{
    QString stylesheet = (status == Svg::Status::Selected)
        ? cachedSelectedSvgStyleSheets.value(theme->cacheKey())
        : (status == Svg::Status::Inactive ? cachedInactiveSvgStyleSheets.value(theme->cacheKey()) : cachedSvgStyleSheets.value(theme->cacheKey()));
    if (stylesheet.isEmpty()) {
        QString skel = QStringLiteral(".ColorScheme-%1{color:%2;}");

        stylesheet += skel.arg(QStringLiteral("Text"), QStringLiteral("%textcolor"));
        stylesheet += skel.arg(QStringLiteral("Background"), QStringLiteral("%backgroundcolor"));

        stylesheet += skel.arg(QStringLiteral("Highlight"), QStringLiteral("%highlightcolor"));
        stylesheet += skel.arg(QStringLiteral("HighlightedText"), QStringLiteral("%highlightedtextcolor"));
        stylesheet += skel.arg(QStringLiteral("PositiveText"), QStringLiteral("%positivetextcolor"));
        stylesheet += skel.arg(QStringLiteral("NeutralText"), QStringLiteral("%neutraltextcolor"));
        stylesheet += skel.arg(QStringLiteral("NegativeText"), QStringLiteral("%negativetextcolor"));

        stylesheet = processStyleSheet(stylesheet, status, theme);
        if (status == Svg::Status::Selected) {
            cachedSelectedSvgStyleSheets.insert(theme->cacheKey(), stylesheet);
        } else if (status == Svg::Status::Inactive) {
            cachedInactiveSvgStyleSheets.insert(theme->cacheKey(), stylesheet);
        } else {
            cachedSvgStyleSheets.insert(theme->cacheKey(), stylesheet);
        }
    }

    return stylesheet;
}

void ThemePrivate::settingsFileChanged(const QString &file)
{
    qCDebug(LOG_KSVG) << "settingsFile: " << file;
    if (file == themeMetadataPath) {
        const KPluginMetaData data = metaDataForTheme(basePath, themeName);
        if (!data.isValid() || themeVersion != data.version()) {
            scheduleThemeChangeNotification(SvgElementsCache);
        }
    } else if (file.endsWith(QLatin1String(themeRcFile))) {
        config().config()->reparseConfiguration();
        settingsChanged(true);
    }
}

void ThemePrivate::settingsChanged(bool emitChanges)
{
    if (fixedName) {
        return;
    }
    // qCDebug(LOG_KSVG) << "Settings Changed!";
    KConfigGroup cg = config();
    setThemeName(cg.readEntry("name", ThemePrivate::defaultTheme), false, emitChanges);
}

bool ThemePrivate::findInCache(const QString &key, QPixmap &pix, unsigned int lastModified)
{
    // TODO KF6: Make lastModified non-optional.
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

void ThemePrivate::insertIntoCache(const QString &key, const QPixmap &pix)
{
    if (useCache()) {
        pixmapCache->insertPixmap(key, pix);
    }
}

void ThemePrivate::insertIntoCache(const QString &key, const QPixmap &pix, const QString &id)
{
    if (useCache()) {
        pixmapsToCache[id] = pix;
        keysToCache[key] = id;
        idsToCache[id] = key;

        // always start timer in pixmapSaveTimer's thread
        QMetaObject::invokeMethod(pixmapSaveTimer, "start", Qt::QueuedConnection);
    }
}

void ThemePrivate::setThemeName(const QString &tempThemeName, bool writeSettings, bool emitChanged)
{
    QString theme = tempThemeName;
    if (theme.isEmpty() || theme == themeName) {
        // let's try and get the default theme at least
        if (themeName.isEmpty()) {
            theme = QLatin1String(ThemePrivate::defaultTheme);
        } else {
            return;
        }
    }

    // we have one special theme: essentially a dummy theme used to cache things with
    // the system colors.
    bool realTheme = theme != QLatin1String(systemColorsTheme);
    if (realTheme) {
        KPluginMetaData data = metaDataForTheme(basePath, theme);
        if (!data.isValid()) {
            data = metaDataForTheme(basePath, QStringLiteral("default"));
            if (!data.isValid()) {
                return;
            }

            theme = QLatin1String(ThemePrivate::defaultTheme);
        }
    }

    // check again as ThemePrivate::defaultTheme might be empty
    if (themeName == theme) {
        return;
    }

    themeName = theme;

    // load the color scheme config
    const QString colorsFile = realTheme ? QStandardPaths::locate(QStandardPaths::GenericDataLocation, basePath % theme % QLatin1String("/colors")) : QString();

    // qCDebug(LOG_KSVG) << "we're going for..." << colorsFile << "*******************";

    if (realTheme) {
        pluginMetaData = metaDataForTheme(basePath, theme);
        KSharedConfigPtr metadata = configForTheme(basePath, theme);

        KConfigGroup cg(metadata, "Settings");
        QString fallback = cg.readEntry("FallbackTheme", QString());

        fallbackThemes.clear();
        while (!fallback.isEmpty() && !fallbackThemes.contains(fallback)) {
            fallbackThemes.append(fallback);

            KSharedConfigPtr metadata = configForTheme(basePath, fallback);
            KConfigGroup cg(metadata, "Settings");
            fallback = cg.readEntry("FallbackTheme", QString());
        }

        if (!fallbackThemes.contains(QLatin1String(ThemePrivate::defaultTheme))) {
            fallbackThemes.append(QLatin1String(ThemePrivate::defaultTheme));
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

    if (realTheme && isDefault && writeSettings) {
        // we're the default theme, let's save our status
        KConfigGroup &cg = config();
        cg.writeEntry("name", themeName);
        cg.sync();
    }

    if (emitChanged) {
        scheduleThemeChangeNotification(PixmapCache | SvgElementsCache);
    }
}

}

#include "moc_theme_p.cpp"
