/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_IMAGESET_H
#define KSVG_IMAGESET_H

#include <QGuiApplication>
#include <QObject>

#include <KSharedConfig>
#include <ksvg/ksvg_export.h>

class KPluginMetaData;

namespace KSvg
{
class ImageSetPrivate;
class SvgPrivate;

// TODO: move in the plasma part the watching and regeneration of icon themes

/**
 * @class ImageSet ksvg/imageset.h <KSvg/ImageSet>
 *
 * @short Interface to the Svg image set
 *
 *
 * KSvg::ImageSet provides access to a common and standardized set of graphic
 * elements stored in SVG format. This allows artists to create single packages
 * of SVGs that will affect the look and feel of all workspace components.
 *
 * KSvg::Svg uses KSvg::ImageSet internally to locate and load the appropriate
 * SVG data. Alternatively, KSvg::ImageSet can be used directly to retrieve
 * file system paths to SVGs by name.
 */
class KSVG_EXPORT ImageSet : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString imageSetName READ imageSetName NOTIFY imageSetChanged)
    Q_PROPERTY(bool useGlobalSettings READ useGlobalSettings NOTIFY imageSetChanged)

public:
    /**
     * Default constructor. It will be the global theme configured in plasmarc
     * @param parent the parent object
     */
    explicit ImageSet(QObject *parent = nullptr);

    /**
     * Construct a theme. It will be a custom theme instance of imageSetName.
     * @param imageSetName the name of the theme to create
     * @param parent the parent object
     */
    explicit ImageSet(const QString &imageSetName, QObject *parent = nullptr);

    ~ImageSet() override;

    /**
     * Sets a base path for the theme to look for svgs.
     * It can be a path relative to QStandardPaths::GenericDataLocation
     * or an absolute path
     *
     * @param basePath the base path
     */
    void setBasePath(const QString &basePath);

    /**
     * @returns the base path of the theme where the svgs will be looked for
     */
    QString basePath() const;

    /**
     * Sets file selectors. The theme can have different svgs with the same name for
     * different situations and platforms.
     * The Plasma desktop for instance uses "opaque" or "translucent"
     * based on presence of compositing and KWin blur effects.
     * Other uses may be platform, like android-specific graphics.
     *
     * @param selectors selectors in order of preference
     */
    void setSelectors(const QStringList &selectors);

    /**
     * @returns the current selectors in order of preference.
     */
    QStringList selectors() const;

    /**
     * Sets the current theme being used.
     */
    void setImageSetName(const QString &imageSetName);

    /**
     * @return the name of the theme.
     */
    QString imageSetName() const;

    /**
     * Retrieve the path for an SVG image in the current theme.
     *
     * @param name the name of the file in the theme directory (without the
     *           ".svg" part or a leading slash)
     * @return the full path to the requested file for the current theme
     */
    QString imagePath(const QString &name) const;

    /**
     * Retrieve the path for a generic file in the current theme.
     * The theme can also ship any generic file, such as configuration files
     *
     * @param name the name of the file in the theme directory (without a leading slash)
     * @return the full path to the requested file for the current theme
     */
    QString filePath(const QString &name) const;

    /**
     * Checks if this theme has an image named in a certain way
     *
     * @param name the name of the file in the theme directory (without the
     *           ".svg" part or a leading slash)
     * @return true if the image exists for this theme
     */
    bool currentImageSetHasImage(const QString &name) const;

    /**
     * Tells the theme whether to follow the global settings or use application
     * specific settings
     *
     * @param useGlobal pass in true to follow the global settings
     */
    void setUseGlobalSettings(bool useGlobal);

    /**
     * @return true if the global settings are followed, false if application
     * specific settings are used.
     */
    bool useGlobalSettings() const;

    /**
     * Sets the maximum size of the cache (in kilobytes). If cache gets bigger
     * the limit then some entries are removed
     * Setting cache limit to 0 disables automatic cache size limiting.
     *
     * Note that the cleanup might not be done immediately, so the cache might
     *  temporarily (for a few seconds) grow bigger than the limit.
     **/
    void setCacheLimit(int kbytes);

    /**
     * @return plugin metadata for this theme, with information such as
     * name, description, author, website etc
     */
    KPluginMetaData metadata() const;

Q_SIGNALS:
    /**
     * Emitted when the user changes the theme. REndered images, colors, etc. should
     * be updated at this point. However, SVGs should *not* be repainted in response
     * to this signal; connect to Svg::repaintNeeded() instead for that, as Svg objects
     * need repainting not only when imageSetChanged() is emitted; moreover Svg objects
     * connect to and respond appropriately to imageSetChanged() internally, emitting
     * Svg::repaintNeeded() at an appropriate time.
     */
    void imageSetChanged();

private:
    friend class SvgPrivate;
    friend class Svg;
    friend class FrameSvg;
    friend class FrameSvgPrivate;
    friend class ImageSetPrivate;
    ImageSetPrivate *d;
};

} // KSvg namespace

#endif // multiple inclusion guard
