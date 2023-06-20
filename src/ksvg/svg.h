/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_SVG_H
#define KSVG_SVG_H

#include <QObject>
#include <QPixmap>

#include <ksvg/imageset.h>
#include <ksvg/ksvg_export.h>

class QPainter;
class QPoint;
class QPointF;
class QRect;
class QRectF;
class QSize;
class QSizeF;
class QMatrix;

/**
 * The KSvg namespace.
 */
namespace KSvg
{
class FrameSvgPrivate;
class SvgPrivate;

/**
 * @class Svg ksvg/svg.h <KSvg/Svg>
 *
 * @short A theme aware image-centric SVG class
 *
 * KSvg::Svg provides a class for rendering SVG images to a QPainter in a
 * convenient manner. Unless an absolute path to a file is provided, it loads
 * the SVG document using KSvg::ImageSet. It also provides a number of internal
 * optimizations to help lower the cost of painting SVGs, such as caching.
 *
 * @see KSvg::FrameSvg
 **/
class KSVG_EXPORT Svg : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSize size READ size WRITE resize NOTIFY sizeChanged)
    Q_PROPERTY(bool multipleImages READ containsMultipleImages WRITE setContainsMultipleImages)
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)
    Q_PROPERTY(bool usingRenderingCache READ isUsingRenderingCache WRITE setUsingRenderingCache)
    Q_PROPERTY(bool fromCurrentImageSet READ fromCurrentImageSet NOTIFY fromCurrentImageSetChanged)
    Q_PROPERTY(KSvg::Svg::Status status READ status WRITE setStatus NOTIFY statusChanged)
    Q_PROPERTY(KSvg::Svg::ColorSet colorSet READ colorSet WRITE setColorSet NOTIFY colorSetChanged)
    // FIXME: here only for compatibility, port away
    Q_PROPERTY(KSvg::Svg::ColorSet colorGroup READ colorSet WRITE setColorSet NOTIFY colorSetChanged)

public:
    enum Status {
        Normal = 0,
        Selected,
        Inactive,
    };
    Q_ENUM(Status)

    // FIXME? Those are copied from KColorScheme because is needed to make it a Q_ENUM
    enum ColorSet { View, Window, Button, Selection, Tooltip, Complementary, Header };
    Q_ENUM(ColorSet)

    enum StyleSheetColor {
        Text,
        Background,
        Highlight,
        HighlightedText,
        PositiveText,
        NeutralText,
        NegativeText,

        ButtonText,
        ButtonBackground,
        ButtonHover,
        ButtonFocus,
        ButtonHighlightedText,
        ButtonPositiveText,
        ButtonNeutralText,
        ButtonNegativeText,

        ViewText,
        ViewBackground,
        ViewHover,
        ViewFocus,
        ViewHighlightedText,
        ViewPositiveText,
        ViewNeutralText,
        ViewNegativeText,

        TooltipText,
        TooltipBackground,
        TooltipHover,
        TooltipFocus,
        TooltipHighlightedText,
        TooltipPositiveText,
        TooltipNeutralText,
        TooltipNegativeText,

        ComplementaryText,
        ComplementaryBackground,
        ComplementaryHover,
        ComplementaryFocus,
        ComplementaryHighlightedText,
        ComplementaryPositiveText,
        ComplementaryNeutralText,
        ComplementaryNegativeText,

        HeaderText,
        HeaderBackground,
        HeaderHover,
        HeaderFocus,
        HeaderHighlightedText,
        HeaderPositiveText,
        HeaderNeutralText,
        HeaderNegativeText
    };
    Q_ENUM(StyleSheetColor);

    /**
     * Constructs an SVG object that implicitly shares and caches rendering.
     *
     * Unlike QSvgRenderer, which this class uses internally,
     * KSvg::Svg represents an image generated from an SVG. As such, it
     * has a related size and transform matrix (the latter being provided
     * by the painter used to paint the image).
     *
     * The size is initialized to be the SVG's native size.
     *
     * @param parent options QObject to parent this to
     *
     * @related KSvg::ImageSet
     */
    explicit Svg(QObject *parent = nullptr);
    ~Svg() override;

    /**
     * Set the device pixel ratio for the Svg. This is the ratio between
     * image pixels and device-independent pixels.
     * The Svg will produce pixmaps scaled by devicePixelRatio, but all the sizes and element
     * rects will not be altered.
     * The default value is 1.0 and the scale will be done rounded to the floor integer
     * Setting it to something more, will make all the elements of this svg appear bigger.
     */
    void setDevicePixelRatio(qreal factor);

    /**
     * @return the device pixel ratio for this Svg.
     */
    qreal devicePixelRatio() const;

    /**
     * Returns a pixmap of the SVG represented by this object.
     *
     * The size of the pixmap will be the size of this Svg object (size())
     * if containsMultipleImages is @c true; otherwise, it will be the
     * size of the requested element after the whole SVG has been scaled
     * to size().
     *
     * @param elementId  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     * @return a QPixmap of the rendered SVG
     */
    Q_INVOKABLE QPixmap pixmap(const QString &elementID = QString());

    /**
     * Returns an image of the SVG represented by this object.
     *
     * The size of the image will be the size of this Svg object (size())
     * if containsMultipleImages is @c true; otherwise, it will be the
     * size of the requested element after the whole SVG has been scaled
     * to size().
     *
     * @param elementId  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     * @return a QPixmap of the rendered SVG
     */
    Q_INVOKABLE QImage image(const QSize &size, const QString &elementID = QString());

    /**
     * Paints all or part of the SVG represented by this object
     *
     * The size of the painted area will be the size of this Svg object
     * (size()) if containsMultipleImages is @c true; otherwise, it will
     * be the size of the requested element after the whole SVG has been
     * scaled to size().
     *
     * @param painter    the QPainter to use
     * @param point      the position to start drawing; the entire svg will be
     *                 drawn starting at this point.
     * @param elementId  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, const QPointF &point, const QString &elementID = QString());

    /**
     * Paints all or part of the SVG represented by this object
     *
     * The size of the painted area will be the size of this Svg object
     * (size()) if containsMultipleImages is @c true; otherwise, it will
     * be the size of the requested element after the whole SVG has been
     * scaled to size().
     *
     * @param painter    the QPainter to use
     * @param x          the horizontal coordinate to start painting from
     * @param y          the vertical coordinate to start painting from
     * @param elementId  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, int x, int y, const QString &elementID = QString());

    /**
     * Paints all or part of the SVG represented by this object
     *
     * @param painter    the QPainter to use
     * @param rect       the rect to draw into; if smaller than the current size
     *                 the drawing is starting at this point.
     * @param elementId  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, const QRectF &rect, const QString &elementID = QString());

    /**
     * Paints all or part of the SVG represented by this object
     *
     * @param painter    the QPainter to use
     * @param x          the horizontal coordinate to start painting from
     * @param y          the vertical coordinate to start painting from
     * @param width      the width of the element to draw
     * @param height     the height of the element do draw
     * @param elementId  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, int x, int y, int width, int height, const QString &elementID = QString());

    /**
     * The size of the SVG.
     *
     * If the SVG has been resized with resize(), that size will be
     * returned; otherwise, the natural size of the SVG will be returned.
     *
     * If containsMultipleImages is @c true, each element of the SVG
     * will be rendered at this size by default.
     *
     * @return the current size of the SVG
     **/
    QSize size() const;

    /**
     * Resizes the rendered image.
     *
     * Rendering will actually take place on the next call to paint.
     *
     * If containsMultipleImages is @c true, each element of the SVG
     * will be rendered at this size by default; otherwise, the entire
     * image will be scaled to this size and each element will be
     * scaled appropriately.
     *
     * @param width   the new width
     * @param height  the new height
     **/
    Q_INVOKABLE void resize(qreal width, qreal height);

    /**
     * Resizes the rendered image.
     *
     * Rendering will actually take place on the next call to paint.
     *
     * If containsMultipleImages is @c true, each element of the SVG
     * will be rendered at this size by default; otherwise, the entire
     * image will be scaled to this size and each element will be
     * scaled appropriately.
     *
     * @param size  the new size of the image
     **/
    Q_INVOKABLE void resize(const QSizeF &size);

    /**
     * Resizes the rendered image to the natural size of the SVG.
     *
     * Rendering will actually take place on the next call to paint.
     **/
    Q_INVOKABLE void resize();

    /**
     * Find the size of a given element.
     *
     * This is the size of the element with ID @p elementId after the SVG
     * has been scaled (see resize()).  Note that this is unaffected by
     * the containsMultipleImages property.
     *
     * @param elementId  the id of the element to check
     * @return the size of a given element, given the current size of the SVG
     **/
    Q_INVOKABLE QSize elementSize(const QString &elementId) const;

    QSize elementSize(QStringView elementId) const;

    /**
     * The bounding rect of a given element.
     *
     * This is the bounding rect of the element with ID @p elementId after
     * the SVG has been scaled (see resize()).  Note that this is
     * unaffected by the containsMultipleImages property.
     *
     * @param elementId  the id of the element to check
     * @return  the current rect of a given element, given the current size of the SVG
     **/
    Q_INVOKABLE QRectF elementRect(const QString &elementId) const;

    QRectF elementRect(QStringView elementId) const;

    /**
     * Check whether an element exists in the loaded SVG.
     *
     * @param elementId  the id of the element to check for
     * @return @c true if the element is defined in the SVG, otherwise @c false
     **/
    Q_INVOKABLE bool hasElement(const QString &elementId) const;

    bool hasElement(QStringView elementId) const;

    /**
     * Check whether this object is backed by a valid SVG file.
     *
     * This method can be expensive as it causes disk access.
     *
     * @return @c true if the SVG file exists and the document is valid,
     *         otherwise @c false.
     **/
    Q_INVOKABLE bool isValid() const;

    /**
     * Set whether the SVG contains a single image or multiple ones.
     *
     * If this is set to @c true, the SVG will be treated as a
     * collection of related images, rather than a consistent
     * drawing.
     *
     * In particular, when individual elements are rendered, this
     * affects whether the elements are resized to size() by default.
     * See paint() and pixmap().
     *
     * @param multiple true if the svg contains multiple images
     */
    void setContainsMultipleImages(bool multiple);

    /**
     * Whether the SVG contains multiple images.
     *
     * If this is @c true, the SVG will be treated as a
     * collection of related images, rather than a consistent
     * drawing.
     *
     * @return @c true if the SVG will be treated as containing
     *         multiple images, @c false if it will be treated
     *         as a coherent image.
     */
    bool containsMultipleImages() const;

    /**
     * Set the SVG file to render.
     *
     * Relative paths are looked for in the current Svg theme,
     * and should not include the file extension (.svg and .svgz
     * files will be searched for).  See ImageSet::imagePath().
     *
     * If the parent object of this Svg is a KSvg::Applet,
     * relative paths will be searched for in the applet's package
     * first.
     *
     * @param svgFilePath  either an absolute path to an SVG file, or
     *                   an image name
     */
    virtual void setImagePath(const QString &svgFilePath);

    /**
     * The SVG file to render.
     *
     * If this SVG is themed, this will be a relative path, and will not
     * include a file extension.
     *
     * @return  either an absolute path to an SVG file, or an image name
     * @see ImageSet::imagePath()
     */
    QString imagePath() const;

    /**
     * Sets whether or not to cache the results of rendering to pixmaps.
     *
     * If the SVG is resized and re-rendered often (and does not keep using the
     * same small set of pixmap dimensions), then it may be less efficient to do
     * disk caching.  A good example might be a progress meter that uses an Svg
     * object to paint itself: the meter will be changing often enough, with
     * enough unpredictability and without re-use of the previous pixmaps to
     * not get a gain from caching.
     *
     * Most Svg objects should use the caching feature, however.
     * Therefore, the default is to use the render cache.
     *
     * @param useCache true to cache rendered pixmaps
     * @since 4.3
     */
    void setUsingRenderingCache(bool useCache);

    /**
     * Whether the rendering cache is being used.
     *
     * @return @c true if the Svg object is using caching for rendering results
     * @since 4.3
     */
    bool isUsingRenderingCache() const;

    /**
     * Whether the current theme has this Svg, without any fallback
     * to the default theme involved
     *
     * @return true if the svg is loaded from the current theme
     * @see ImageSet::currentImageSetHasImage
     */
    bool fromCurrentImageSet() const;

    /**
     * Sets whether the Svg uses the global system theme for its colors or
     * the Svg theme. Default is False.
     *
     * @since 5.16
     */
    void setUseSystemColors(bool system);

    /**
     * @returns True if colors from the system theme are used.
     *           Default is False
     * @since 5.16
     */
    bool useSystemColors() const;

    /**
     * Sets the KSvg::ImageSet to use with this Svg object.
     *
     * By default, Svg objects use KSvg::ImageSet::default().
     *
     * This determines how relative image paths are interpreted.
     *
     * @param theme  the theme object to use
     * @since 4.3
     */
    void setImageSet(KSvg::ImageSet *theme);

    /**
     * The KSvg::ImageSet used by this Svg object.
     *
     * This determines how relative image paths are interpreted.
     *
     * @return  the theme used by this Svg
     */
    ImageSet *imageSet() const;

    /**
     * Sets the image in a selected status.
     * Svgs can be colored with system color themes, if the status is selected,
     * the TextColor will become HighlightedText color and BackgroundColor
     * will become HighlightColor, making the svg graphics (for instance an icon)
     * will look correct together selected text
     * Supported statuses as of 5.23 are Normal and Selected
     * @since 5.23
     */
    void setStatus(Svg::Status status);

    /**
     * @return the status of the Svg
     * @since 5.23
     */
    Svg::Status status() const;

    /**
     * Set a color set for the Svg.
     * if the Svg uses stylesheets and has elements
     * that are either TextColor or BackgroundColor class,
     * make them use ButtonTextColor/ButtonBackgroundColor
     * or ViewTextColor/ViewBackgroundColor
     */
    void setColorSet(ColorSet colorSet);

    /**
     * @return the color set for this Svg
     */
    KSvg::Svg::ColorSet colorSet() const;

    QColor color(StyleSheetColor colorName) const;
    void setColor(StyleSheetColor colorName, const QColor &color);

    void clearColorOverrides();

Q_SIGNALS:
    /**
     * Emitted whenever the SVG data has changed in such a way that a repaint is required.
     * Any usage of an Svg object that does the painting itself must connect to this signal
     * and respond by updating the painting. Note that connected to ImageSet::imageSetChanged is
     * incorrect in such a use case as the Svg itself may not be updated yet nor may theme
     * change be the only case when a repaint is needed. Also note that classes or QML code
     * which take Svg objects as parameters for their own painting all respond to this signal
     * so that in those cases manually responding to the signal is unnecessary; ONLY when
     * direct, manual painting with an Svg object is done in application code is this signal
     * used.
     */
    void repaintNeeded();

    /**
     * Emitted whenever the size of the Svg is changed. @see resize()
     */
    void sizeChanged();

    /**
     * Emitted whenever the image path of the Svg is changed.
     */
    void imagePathChanged();

    /**
     * Emitted whenever the color hint has changed.
     */
    void colorHintChanged();

    /**
     * Emitted when fromCurrentImageSet() value has changed
     */
    void fromCurrentImageSetChanged(bool fromCurrentImageSet);

    /**
     * Emitted when the status changes
     */
    void statusChanged(KSvg::Svg::Status status);

    /**
     * Emitted when the color set changes
     */
    void colorSetChanged(KSvg::Svg::ColorSet colorSet);

    /**
     * Emitted when the image set of this svg has changed
     */
    void imageSetChanged(ImageSet *imageSet);

private:
    SvgPrivate *const d;
    bool eventFilter(QObject *watched, QEvent *event) override;

    Q_PRIVATE_SLOT(d, void imageSetChanged())
    Q_PRIVATE_SLOT(d, void colorsChanged())

    friend class SvgPrivate;
    friend class FrameSvgPrivate;
    friend class FrameSvg;
    friend class ImageSetPrivate;
};

} // KSvg namespace

#endif // multiple inclusion guard
