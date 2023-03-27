/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2013 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_THEME_H
#define KSVG_THEME_H

#include <QGuiApplication>
#include <QObject>

#include <KSharedConfig>
#include <ksvg/ksvg_export.h>

class KPluginMetaData;

namespace KSvg
{
class ThemePrivate;
class SvgPrivate;

/**
 * @class Theme ksvg/theme.h <KSvg/Theme>
 *
 * @short Interface to the Svg theme
 *
 *
 * KSvg::Theme provides access to a common and standardized set of graphic
 * elements stored in SVG format. This allows artists to create single packages
 * of SVGs that will affect the look and feel of all workspace components.
 *
 * KSvg::Svg uses KSvg::Theme internally to locate and load the appropriate
 * SVG data. Alternatively, KSvg::Theme can be used directly to retrieve
 * file system paths to SVGs by name.
 */
class KSVG_EXPORT Theme : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString themeName READ themeName NOTIFY themeChanged)
    Q_PROPERTY(bool useGlobalSettings READ useGlobalSettings NOTIFY themeChanged)

    Q_PROPERTY(QPalette palette READ palette NOTIFY themeChanged)

public:
    enum DisplayHint { Normal = 0, Opaque, Translucent };

    enum ColorRole {
        TextColor = 0, /**<  the text color to be used by items resting on the background */
        BackgroundColor = 1, /**< the default background color */
        HighlightColor = 2, /**<  the text highlight color to be used by items resting
                                   on the background */
        HoverColor = 3, /**< color for hover effect on view */
        FocusColor = 4, /**< color for focus effect on view */
        LinkColor = 5, /**< color for clickable links */
        VisitedLinkColor = 6, /**< color visited clickable links */
        HighlightedTextColor = 7, /**< color contrasting with HighlightColor, to be used for instance with */
        PositiveTextColor = 8, /**< color of foreground objects with a "positive message" connotation (usually green) */
        NeutralTextColor = 9, /**< color of foreground objects with a "neutral message" connotation (usually yellow) */
        NegativeTextColor = 10, /**< color of foreground objects with a "negative message" connotation (usually red) */
        DisabledTextColor = 11, /**< color of disabled text @since 5.64 */
    };

    enum ColorGroup {
        NormalColorGroup = 0,
        ButtonColorGroup = 1,
        ViewColorGroup = 2,
        ComplementaryColorGroup = 3,
        HeaderColorGroup,
        ToolTipColorGroup,
    };
    Q_ENUM(ColorGroup)

    /**
     * Default constructor. It will be the global theme configured in plasmarc
     * @param parent the parent object
     */
    explicit Theme(QObject *parent = nullptr);

    /**
     * Construct a theme. It will be a custom theme instance of themeName.
     * @param themeName the name of the theme to create
     * @param parent the parent object
     * @since 4.3
     */
    explicit Theme(const QString &themeName, QObject *parent = nullptr);

    ~Theme() override;

    void setBasePath(const QString &basePath);
    QString basePath() const;

    void setSelectors(const QStringList &selectors);

    QStringList selectors() const;

    /**
     * Sets the current theme being used.
     */
    void setThemeName(const QString &themeName);

    /**
     * @return the name of the theme.
     */
    QString themeName() const;

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
    bool currentThemeHasImage(const QString &name) const;

    /**
     * Returns the color scheme configurationthat goes along this theme.
     * This can be used with KStatefulBrush and KColorScheme to determine
     * the proper colours to use along with the visual elements in this theme.
     */
    KSharedConfigPtr colorScheme() const;

    /**
     * Returns the text color to be used by items resting on the background
     *
     * @param role which role (usage pattern) to get the color for
     * @param group which group we want a color of
     */
    QColor color(ColorRole role, ColorGroup group = NormalColorGroup) const;

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
     * Returns a QPalette with the colors set as defined by the theme.
     * @since 5.68
     */
    QPalette palette() const;

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
     * @since 5.95
     */
    KPluginMetaData metadata() const;

    static QPalette globalPalette();

Q_SIGNALS:
    /**
     * Emitted when the user changes the theme. REndered images, colors, etc. should
     * be updated at this point. However, SVGs should *not* be repainted in response
     * to this signal; connect to Svg::repaintNeeded() instead for that, as Svg objects
     * need repainting not only when themeChanged() is emitted; moreover Svg objects
     * connect to and respond appropriately to themeChanged() internally, emitting
     * Svg::repaintNeeded() at an appropriate time.
     */
    void themeChanged();

private:
    friend class SvgPrivate;
    friend class FrameSvg;
    friend class FrameSvgPrivate;
    friend class ThemePrivate;
    ThemePrivate *d;
};

} // KSvg namespace

#endif // multiple inclusion guard
