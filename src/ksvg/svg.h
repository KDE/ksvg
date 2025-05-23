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

/*!
 * \namespace KSvg
 * \inmodule KSvg
 * \brief The KSvg namespace.
 */
namespace KSvg
{
class FrameSvgPrivate;
class SvgPrivate;

/*!
 * \class KSvg::Svg
 * \inheaderfile KSvg/Svg
 * \inmodule KSvg
 *
 * \brief A theme aware image-centric SVG class.
 *
 * KSvg::Svg provides a class for rendering SVG images to a QPainter in a
 * convenient manner. Unless an absolute path to a file is provided, it loads
 * the SVG document using KSvg::ImageSet. It also provides a number of internal
 * optimizations to help lower the cost of painting SVGs, such as caching.
 *
 * \sa KSvg::FrameSvg
 **/
class KSVG_EXPORT Svg : public QObject
{
    Q_OBJECT

    /*!
     * \property KSvg::Svg::size
     */
    Q_PROPERTY(QSizeF size READ size WRITE resize NOTIFY sizeChanged)

    /*!
     * \property KSvg::Svg::multipleImages
     */
    Q_PROPERTY(bool multipleImages READ containsMultipleImages WRITE setContainsMultipleImages)

    /*!
     * \property KSvg::Svg::imagePath
     */
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)

    /*!
     * \property KSvg::Svg::usingRenderingCache
     */
    Q_PROPERTY(bool usingRenderingCache READ isUsingRenderingCache WRITE setUsingRenderingCache)

    /*!
     * \property KSvg::Svg::fromCurrentImageSet
     */
    Q_PROPERTY(bool fromCurrentImageSet READ fromCurrentImageSet NOTIFY fromCurrentImageSetChanged)

    /*!
     * \property KSvg::Svg::status
     */
    Q_PROPERTY(KSvg::Svg::Status status READ status WRITE setStatus NOTIFY statusChanged)

    /*!
     * \property KSvg::Svg::colorSet
     */
    Q_PROPERTY(KSvg::Svg::ColorSet colorSet READ colorSet WRITE setColorSet NOTIFY colorSetChanged)

public:
    /*!
     * \enum KSvg::Svg::Status
     * \value Normal
     * \value Selected
     * \value Inactive
     */
    enum Status {
        Normal = 0,
        Selected,
        Inactive,
    };
    Q_ENUM(Status)

    // FIXME? Those are copied from KColorScheme because is needed to make it a Q_ENUM
    /*!
     * \enum KSvg::Svg::ColorSet
     *
     * \value View
     * \value Window
     * \value Button
     * \value Selection
     * \value Tooltip
     * \value Complementary
     * \value Header
     */
    enum ColorSet { View, Window, Button, Selection, Tooltip, Complementary, Header };
    Q_ENUM(ColorSet)

    /*!
     * \enum KSvg::Svg::StyleSheetColor
     * \value Text
     * \value Background
     * \value Highlight
     * \value HighlightedText
     * \value PositiveText
     * \value NeutralText
     * \value NegativeText
     * \value ButtonText
     * \value ButtonBackground
     * \value ButtonHover
     * \value ButtonFocus
     * \value ButtonHighlightedText
     * \value ButtonPositiveText
     * \value ButtonNeutralText
     * \value ButtonNegativeText
     * \value ViewText
     * \value ViewBackground
     * \value ViewHover
     * \value ViewFocus
     * \value ViewHighlightedText
     * \value ViewPositiveText
     * \value ViewNeutralText
     * \value ViewNegativeText
     * \value TooltipText
     * \value TooltipBackground
     * \value TooltipHover
     * \value TooltipFocus
     * \value TooltipHighlightedText
     * \value TooltipPositiveText
     * \value TooltipNeutralText
     * \value TooltipNegativeText
     * \value ComplementaryText
     * \value ComplementaryBackground
     * \value ComplementaryHover
     * \value ComplementaryFocus
     * \value ComplementaryHighlightedText
     * \value ComplementaryPositiveText
     * \value ComplementaryNeutralText
     * \value ComplementaryNegativeText
     * \value HeaderText
     * \value HeaderBackground
     * \value HeaderHover
     * \value HeaderFocus
     * \value HeaderHighlightedText
     * \value HeaderPositiveText
     * \value HeaderNeutralText
     * \value HeaderNegativeText
     */
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

    /*!
     * \brief This method constructs an SVG object that implicitly shares and
     * caches rendering.
     *
     * Unlike QSvgRenderer, which this class uses internally,
     * KSvg::Svg represents an image generated from an SVG. As such, it has a
     * related size and transform matrix (the latter being provided by the
     * painter used to paint the image).
     *
     * The size is initialized to be the SVG's native size.
     *
     * \a parent options QObject to parent this to
     *
     * \sa KSvg::ImageSet
     */
    explicit Svg(QObject *parent = nullptr);
    ~Svg() override;

    /*!
     * \brief This method sets the device pixel ratio for the Svg.
     *
     * This is the ratio between image pixels and device-independent pixels. The
     * SVG will produce pixmaps scaled by devicePixelRatio, but all the sizes
     * and element rects will not be altered. The default value is 1.0 and the
     * scale will be done rounded to the floor integer.
     *
     * Setting it to something higher will make all the elements of this SVG
     * appear bigger.
     */
    void setDevicePixelRatio(qreal factor);

    /*!
     * \brief This method returns the device pixel ratio for this Svg.
     */
    qreal devicePixelRatio() const;

    /*!
     * \brief This method returns a pixmap of the SVG represented by this
     * object.
     *
     * The size of the pixmap will be the size of this Svg object (size()) if
     * containsMultipleImages is \c true; otherwise, it will be the size of the
     * requested element after the whole SVG has been scaled to size().
     *
     * \a elementID  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     *
     * Returns a QPixmap of the rendered SVG
     */
    Q_INVOKABLE QPixmap pixmap(const QString &elementID = QString());

    /*!
     * \brief This method returns an image of the SVG represented by this
     * object.
     *
     * The size of the image will be the size of this Svg object (size()) if
     * containsMultipleImages is \c true; otherwise, it will be the size of the
     * requested element after the whole SVG has been scaled to size().
     *
     * \a elementID the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     *
     * Returns a QPixmap of the rendered SVG
     */
    Q_INVOKABLE QImage image(const QSize &size, const QString &elementID = QString());

    /*!
     * \brief This method paints all or part of the SVG represented by this
     * object.
     *
     * The size of the painted area will be the size of this Svg object (size())
     * if containsMultipleImages is \c true; otherwise, it will be the size of
     * the requested element after the whole SVG has been scaled to size().
     *
     * \a painter    the QPainter to use
     *
     * \a point      the position to start drawing; the entire svg will be
     *                 drawn starting at this point.
     *
     * \a elementID  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, const QPointF &point, const QString &elementID = QString());

    /*!
     * \brief This method paints all or part of the SVG represented by this
     * object.
     *
     * The size of the painted area will be the size of this Svg object (size())
     * if containsMultipleImages is \c true; otherwise, it will be the size of
     * the requested element after the whole SVG has been scaled to size().
     *
     * \a painter    the QPainter to use
     *
     * \a x          the horizontal coordinate to start painting from
     *
     * \a y          the vertical coordinate to start painting from
     *
     * \a elementID  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, int x, int y, const QString &elementID = QString());

    /*!
     * \brief This method paints all or part of the SVG represented by this
     * object.
     *
     * \a painter    the QPainter to use
     *
     * \a rect       the rect to draw into; if smaller than the current size
     *                 the drawing is starting at this point.
     *
     * \a elementID  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, const QRectF &rect, const QString &elementID = QString());

    /*!
     * \brief This method paints all or part of the SVG represented by this
     * object.
     *
     * \a painter    the QPainter to use
     *
     * \a x          the horizontal coordinate to start painting from
     *
     * \a y          the vertical coordinate to start painting from
     *
     * \a width      the width of the element to draw
     *
     * \a height     the height of the element do draw
     *
     * \a elementID  the ID string of the element to render, or an empty
     *                 string for the whole SVG (the default)
     */
    Q_INVOKABLE void paint(QPainter *painter, int x, int y, int width, int height, const QString &elementID = QString());

    /*!
     * \brief This method returns the size of the SVG.
     *
     * If the SVG has been resized with resize(), that size will be returned;
     * otherwise, the natural size of the SVG will be returned.
     *
     * If containsMultipleImages is \c true, each element of the SVG will be
     * rendered at this size by default.
     *
     * Returns the current size of the SVG
     **/
    QSizeF size() const;

    /*!
     * \brief This method resizes the rendered image.
     *
     * Rendering will actually take place on the next call to paint.
     *
     * If containsMultipleImages is \c true, each element of the SVG will be
     * rendered at this size by default; otherwise, the entire image will be
     * scaled to this size and each element will be scaled appropriately.
     *
     * \a width   the new width
     *
     * \a height  the new height
     **/
    Q_INVOKABLE void resize(qreal width, qreal height);

    /*!
     * \brief This method resizes the rendered image.
     *
     * Rendering will actually take place on the next call to paint.
     *
     * If containsMultipleImages is \c true, each element of the SVG will be
     * rendered at this size by default; otherwise, the entire image will be
     * scaled to this size and each element will be scaled appropriately.
     *
     * \a size  the new size of the image
     **/
    Q_INVOKABLE void resize(const QSizeF &size);

    /*!
     * \brief This method resizes the rendered image to the natural size of the
     * SVG.
     *
     * Rendering will actually take place on the next call to paint.
     **/
    Q_INVOKABLE void resize();

    /*!
     * \brief This method returns the size of a given element.
     *
     * This is the size of the element with ID \a elementId after the SVG
     * has been scaled (see resize()).  Note that this is unaffected by
     * the containsMultipleImages property.
     *
     * \a elementId  the id of the element to check
     *
     * Returns the size of a given element, given the current size of the SVG
     **/
    Q_INVOKABLE QSizeF elementSize(const QString &elementId) const;

    QSizeF elementSize(QStringView elementId) const;

    /*!
     * \brief This method returns the bounding rect of a given element.
     *
     * This is the bounding rect of the element with ID \a elementId after the
     * SVG has been scaled (see resize()).  Note that this is unaffected by the
     * containsMultipleImages property.
     *
     * \a elementId  the id of the element to check
     *
     * Returns  the current rect of a given element, given the current size of the SVG
     **/
    Q_INVOKABLE QRectF elementRect(const QString &elementId) const;

    /*!
     *
     */
    QRectF elementRect(QStringView elementId) const;

    /*!
     * \brief This method checks whether an element exists in the loaded SVG.
     *
     * \a elementId  the id of the element to check for
     *
     * Returns \c true if the element is defined in the SVG, otherwise \c false
     **/
    Q_INVOKABLE bool hasElement(const QString &elementId) const;

    /*!
     *
     */
    bool hasElement(QStringView elementId) const;

    /*!
     * \brief This method checks whether this object is backed by a valid SVG
     * file.
     *
     * This method can be expensive as it causes disk access.
     *
     * Returns \c true if the SVG file exists and the document is valid,
     * otherwise \c false.
     **/
    Q_INVOKABLE bool isValid() const;

    /*!
     * \brief This method sets whether the SVG contains a single image or
     * multiple ones.
     *
     * If this is set to \c true, the SVG will be treated as a collection of
     * related images, rather than a consistent drawing.
     *
     * In particular, when individual elements are rendered, this affects
     * whether the elements are resized to size() by default. See paint() and
     * pixmap().
     *
     * \sa paint()
     * \sa pixmap()
     *
     * \a multiple true if the svg contains multiple images
     */
    void setContainsMultipleImages(bool multiple);

    /*!
     * \brief This method returns whether the SVG contains multiple images.
     *
     * If this is \c true, the SVG will be treated as a collection of related
     * images, rather than a consistent drawing.
     *
     * Returns \c true if the SVG will be treated as containing multiple images,
     * \c false if it will be treated as a coherent image.
     */
    bool containsMultipleImages() const;

    /*!
     * \brief This method sets the SVG file to render.
     *
     * Relative paths are looked for in the current Svg theme, and should not
     * include the file extension (.svg and .svgz files will be searched for).
     * include the file extension; files with the .svg and .svgz extensions will be
     * found automatically.
     *
     * \sa ImageSet::imagePath()
     *
     * If the parent object of this Svg is a KSvg::Applet, relative paths will
     * be searched for in the applet's package first.
     *
     * \a svgFilePath either an absolute path to an SVG file, or an image
     * name.
     */
    virtual void setImagePath(const QString &svgFilePath);

    /*!
     * \brief This method returns the SVG file to render.
     *
     * If this SVG is themed, this will be a relative path, and will not
     * include a file extension.
     *
     * Returns  either an absolute path to an SVG file, or an image name
     * \sa ImageSet::imagePath()
     */
    QString imagePath() const;

    /*!
     * \brief This method sets whether or not to cache the results of rendering
     * to pixmaps.
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
     * \a useCache true to cache rendered pixmaps
     * \since 4.3
     */
    void setUsingRenderingCache(bool useCache);

    /*!
     * Whether the rendering cache is being used.
     *
     * \brief This method returns whether the Svg object is using caching for
     * rendering results.
     *
     * \since 4.3
     */
    bool isUsingRenderingCache() const;

    /*!
     * \brief This method returns whether the current theme has this SVG,
     * without having to fall back to the default theme.
     *
     * Returns true if the svg is loaded from the current theme
     * \sa ImageSet::currentImageSetHasImage
     */
    bool fromCurrentImageSet() const;

    /*!
     * \brief This method sets the KSvg::ImageSet to use with this Svg object.
     *
     * By default, Svg objects use KSvg::ImageSet::default().
     *
     * This determines how relative image paths are interpreted.
     *
     * \a theme  the theme object to use
     * \since 4.3
     */
    void setImageSet(KSvg::ImageSet *theme);

    /*!
     * \brief This method returns the KSvg::ImageSet used by this Svg object.
     *
     * This determines how relative image paths are interpreted.
     *
     * Returns  the theme used by this Svg
     */
    ImageSet *imageSet() const;

    /*!
     * \brief This method sets the image in a selected status.
     *
     * SVGs can be colored with system color themes. If \a status is selected,
     * \c TextColor will become \c HighlightedText color, and \c BackgroundColor will
     * become \c HighlightColor. This can be used to make SVG-based graphics such
     * as symbolic icons look correct together. Supported statuses are \c Normal
     * and \c Selected.
     * \since 5.23
     */
    void setStatus(Svg::Status status);

    /*!
     * \brief This method returns the Svg object's status.
     * \since 5.23
     */
    Svg::Status status() const;

    /*!
     * \brief This method sets a color set for the SVG.
     * Set a color set for the Svg.
     * if the Svg uses stylesheets and has elements
     * that are either \c TextColor or \c BackgroundColor class,
     * make them use \c ButtonTextColor / \c ButtonBackgroundColor
     * or \c ViewTextColor / \c ViewBackgroundColor
     */
    void setColorSet(ColorSet colorSet);

    /*!
     * Returns the color set for this Svg
     */
    KSvg::Svg::ColorSet colorSet() const;

    /*!
     *
     */
    QColor color(StyleSheetColor colorName) const;

    /*!
     *
     */
    void setColor(StyleSheetColor colorName, const QColor &color);

    /*!
     *
     */
    void clearColorOverrides();

Q_SIGNALS:
    /*!
     * \brief This signal is emitted whenever the SVG data has changed in such a
     * way that a repaint is required.
     *
     * Any usage of an SVG object that does the painting itself must connect to
     * this signal and respond by updating the painting. Note that connecting to
     * ImageSet::imageSetChanged is incorrect in such a use case as the SVG
     * itself may not be updated yet nor may theme change be the only case when
     * a repaint is needed. Also note that classes or QML code which take Svg
     * objects as parameters for their own painting all respond to this signal
     * so that in those cases manually responding to the signal is unnecessary;
     * ONLY when direct, manual painting with an Svg object is done in
     * application code is this signal used.
     */
    void repaintNeeded();

    /*!
     * \brief This signal is emitted whenever the size has changed.
     * \sa resize()
     */
    void sizeChanged();

    /*!
     * \brief This signal is emitted whenever the image path has changed.
     */
    void imagePathChanged();

    /*!
     * \brief This signal is emitted whenever the color hint has changed.
     */
    void colorHintChanged();

    /*!
     * \brief This signal is emitted when the value of fromCurrentImageSet()
     * has changed.
     */
    void fromCurrentImageSetChanged(bool fromCurrentImageSet);

    /*!
     * \brief This signal is emitted when the status has changed.
     */
    void statusChanged(KSvg::Svg::Status status);

    /*!
     * \brief This signal is emitted when the color set has changed.
     */
    void colorSetChanged(KSvg::Svg::ColorSet colorSet);

    /*!
     * \brief This signal is emitted when the image set has changed.
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
