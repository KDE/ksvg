/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2008 Marco Martin <notmart@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef KSVG_FRAMESVG_H
#define KSVG_FRAMESVG_H

#include <QObject>
#include <QPixmap>

#include <ksvg/ksvg_export.h>

#include <ksvg/svg.h>

class QPainter;
class QPoint;
class QPointF;
class QRect;
class QRectF;
class QSize;
class QSizeF;
class QMatrix;

namespace KSvg
{
class FrameSvgPrivate;

/*!
 * \class KSvg::FrameSvg
 * \inheaderfile KSvg/FrameSvg
 * \inmodule KSvg
 *
 * \brief Provides an SVG with borders.
 *
 * When using SVG images for a background of an object that may change
 * its aspect ratio, such as a dialog, simply scaling a single image
 * may not be enough.
 *
 * FrameSvg allows SVGs to provide several elements for borders as well
 * as a central element, each of which are scaled individually. These elements
 * should be named:
 * \list
 * \li \c center  - the central element, which will be scaled in both directions
 * \li \c top     - the top border; the height is fixed, but it will be scaled
 * horizontally to the same width as \c center
 * \li \c bottom  - the bottom border; scaled in the same way as \c top
 * \li \c left    - the left border; the width is fixed, but it will be scaled
 * vertically to the same height as \c center
 * \li \c right   - the right border; scaled in the same way as \c left
 * \li \c topleft - fixed size; must be the same height as \c top and the same
 * width as \c left
 * \li \c bottomleft, \c topright, \c bottomright - similar to \c topleft
 * \endlist
 *
 * \c center must exist, but all the others are optional.  \c topleft and
 * \c topright will be ignored if \c top does not exist, and similarly for
 * \c bottomleft and \c bottomright.
 *
 * \sa KSvg::Svg
 **/
class KSVG_EXPORT FrameSvg : public Svg
{
    Q_OBJECT

    /*!
     * \property KSvg::FrameSvg::enabledBorders
     */
    Q_PROPERTY(EnabledBorders enabledBorders READ enabledBorders WRITE setEnabledBorders)

public:
    /*!
     * \enum KSvg::FrameSvg::EnabledBorder
     *
     * \brief This flag enum specifies which borders should be drawn.
     *
     * \value NoBorder
     * \value TopBorder
     * \value BottomBorder
     * \value LeftBorder
     * \value RightBorder
     * \value AllBorders
     */
    enum EnabledBorder {
        NoBorder = 0,
        TopBorder = 1,
        BottomBorder = 2,
        LeftBorder = 4,
        RightBorder = 8,
        AllBorders = TopBorder | BottomBorder | LeftBorder | RightBorder,
    };
    Q_DECLARE_FLAGS(EnabledBorders, EnabledBorder)
    Q_FLAG(EnabledBorders)

    // TODO: merge those two?
    /*!
     * \enum KSvg::FrameSvg::LocationPrefix
     *
     * \value Floating Free floating.
     * \value TopEdge Along the top of the screen.
     * \value BottomEdge Along the bottom of the screen.
     * \value LeftEdge Along the left side of the screen.
     * \value RightEdge Along the right side of the screen.
     */
    enum LocationPrefix {
        Floating = 0,
        TopEdge,
        BottomEdge,
        LeftEdge,
        RightEdge,
    };
    Q_ENUM(LocationPrefix)

    /*!
     * \enum KSvg::FrameSvg::MarginEdge
     *
     * \value TopMargin The top margin.
     * \value BottomMargin The bottom margin.
     * \value LeftMargin The left margin.
     * \value RightMargin The right margin.
     */
    enum MarginEdge {
        TopMargin = 0,
        BottomMargin,
        LeftMargin,
        RightMargin,
    };
    Q_ENUM(MarginEdge)

    /*!
     * Constructs a new FrameSvg that paints the proper named subelements
     * as borders. It may also be used as a regular KSvg::Svg object
     * for direct access to elements in the Svg.
     *
     * \a parent options QObject to parent this to
     *
     * \sa KSvg::Theme
     */
    explicit FrameSvg(QObject *parent = nullptr);
    ~FrameSvg() override;

    /*!
     * Loads a new Svg
     * \a imagePath the new file
     */
    Q_INVOKABLE void setImagePath(const QString &path) override;

    /*!
     * \brief This method sets which borders should be painted.
     *
     * \a borders the borders we want to paint
     *
     * \sa EnabledBorder
     */
    void setEnabledBorders(const EnabledBorders borders);

    /*!
     * \brief This is a convenience method to get the enabled borders.
     * Returns what borders are painted
     */
    EnabledBorders enabledBorders() const;

    /*!
     * \brief This method resizes the frame, maintaining the same border size.
     *
     * \a size the new size of the frame
     */
    Q_INVOKABLE void resizeFrame(const QSizeF &size);

    /*!
     * Returns the size of the frame
     */
    Q_INVOKABLE QSizeF frameSize() const;

    /*!
     *
     * \brief This method returns the margin size for the given edge.
     *
     * Note that \c 0 will be returned if the given margin is disabled.
     *
     * If you don't care about the margin being on or off, use
     * fixedMarginSize() instead.
     *
     * \a edge the margin edge we want, top, bottom, left or right
     *
     * Returns the margin size
     */
    Q_INVOKABLE qreal marginSize(const FrameSvg::MarginEdge edge) const;

    /*!
     * \brief This is a convenience method that extracts the size of the four
     * margins and saves their size into the passed variables.
     *
     * If you don't care about the margins being on or off, use
     * getFixedMargins() instead.
     *
     * \a left left margin size
     *
     * \a top top margin size
     *
     * \a right right margin size
     *
     * \a bottom bottom margin size
     */
    Q_INVOKABLE void getMargins(qreal &left, qreal &top, qreal &right, qreal &bottom) const;

    /*!
     * \brief This method returns the margin size for the specified edge.
     *
     * Compared to marginSize(), this does not depend on whether the margin is
     * enabled or not.
     *
     * \a edge the margin edge we want, top, bottom, left or right
     * Returns the margin size
     */
    Q_INVOKABLE qreal fixedMarginSize(const FrameSvg::MarginEdge edge) const;

    /*!
     * \brief This is a convenience method that extracts the size of the four
     * margins and saves their size into the passed variables.
     *
     * Compared to getMargins(), this doesn't depend on whether the margins are
     * enabled or not.
     *
     * \a left left margin size
     *
     * \a top top margin size
     *
     * \a right right margin size
     *
     * \a bottom bottom margin size
     */
    Q_INVOKABLE void getFixedMargins(qreal &left, qreal &top, qreal &right, qreal &bottom) const;

    /*!
     * \brief This method returns the insets margin size for the specified edge.
     *
     * \a edge the margin edge we want, top, bottom, left or right
     *
     * Returns the margin size
     * \since 5.77
     */
    Q_INVOKABLE qreal insetSize(const FrameSvg::MarginEdge edge) const;

    /*!
     * \brief This is a convenience method that extracts the size of the four
     * inset margins and saves their size into the passed variables.
     *
     * \a left left margin size
     *
     * \a top top margin size
     *
     * \a right right margin size
     *
     * \a bottom bottom margin size
     * \since 5.77
     */
    Q_INVOKABLE void getInset(qreal &left, qreal &top, qreal &right, qreal &bottom) const;

    /*!
     * \brief This method returns the rectangle of the center element, taking
     * the margins into account.
     */
    Q_INVOKABLE QRectF contentsRect() const;

    /*!
     * \brief This method sets the prefix to 'north',
     * 'south', 'west' and 'east' when the location is TopEdge, BottomEdge,
     * LeftEdge and RightEdge, respectively. Clears the prefix in other cases.
     *
     * The prefix must exist in the SVG document, which means that this can only
     * be called successfully after setImagePath is called.
     *
     * \a location location in the UI this frame will be drawn
     *
     * \sa setElementPrefix
     */
    Q_INVOKABLE void setElementPrefix(KSvg::FrameSvg::LocationPrefix location);

    /*!
     * \brief This method sets the prefix for the SVG elements to be used for
     * painting.
     *
     * For example, if prefix is 'active', then instead of using the 'top'
     * element of the SVG file to paint the top border, the 'active-top' element
     * will be used. The same goes for other SVG elements.
     *
     * If the elements with prefixes are not present, the default ones are used.
     * (for the sake of speed, the test is present only for the 'center' element)
     *
     * Setting the prefix manually resets the location to Floating.
     *
     * The prefix must exist in the SVG document, which means that this can only be
     * called successfully after setImagePath is called.
     *
     * \a prefix prefix for the SVG elements that make up the frame
     */
    Q_INVOKABLE void setElementPrefix(const QString &prefix);

    /*!
     * \brief This method returns whether the SVG has the necessary elements
     * with the given prefix to draw a frame.
     *
     * \a prefix the given prefix we want to check if drawable (can have trailing '-' since 5.59)
     */
    Q_INVOKABLE bool hasElementPrefix(const QString &prefix) const;

    /*!
     * \brief This is an overloaded method provided for convenience that is
     * equivalent to hasElementPrefix("north"), hasElementPrefix("south")
     * hasElementPrefix("west") and hasElementPrefix("east").
     *
     * Returns true if the svg has the necessary elements with the given prefix
     * to draw a frame.
     *
     * \a location the given prefix we want to check if drawable
     */
    Q_INVOKABLE bool hasElementPrefix(KSvg::FrameSvg::LocationPrefix location) const;

    /*!
     * \brief This method returns the prefix for SVG elements of the FrameSvg
     * (including a '-' at the end if not empty).
     *
     * Returns the prefix
     * \sa actualPrefix()
     */
    Q_INVOKABLE QString prefix();

    /*!
     * \brief This method returns a mask that tightly contains the fully opaque
     * areas of the SVG.
     *
     * Returns a region of opaque areas
     */
    Q_INVOKABLE QRegion mask() const;

    /*!
     * \brief This method returns a pixmap whose alpha channel is the opacity of
     * the frame. It may be the frame itself or a special frame with the
     * "mask-" prefix.
     */
    QPixmap alphaMask() const;

    /*!
     * \brief This method sets whether saving all the rendered prefixes in a
     * cache or not.
     *
     * \a cache whether to use the cache.
     */
    Q_INVOKABLE void setCacheAllRenderedFrames(bool cache);

    /*!
     * \brief This method returns whether all the different prefixes should be
     * kept in a cache when rendered.
     */
    Q_INVOKABLE bool cacheAllRenderedFrames() const;

    /*!
     * \brief This method deletes the internal cache.
     *
     * Calling this method frees memory. Use this if you want to switch the
     * rendered element and you don't plan to switch back to the previous one
     * for a long time.
     *
     * This only works if setUsingRenderingCache(\c true) has been called.
     *
     * \sa KSvg::Svg::setUsingRenderingCache()
     */
    Q_INVOKABLE void clearCache();

    /*!
     * \brief This method returns a pixmap of the SVG represented by this
     * object.
     *
     * Returns a QPixmap of the rendered SVG
     */
    Q_INVOKABLE QPixmap framePixmap();

    /*!
     * \brief This method paints the loaded SVG with the elements that
     * represents the border.
     *
     * \a painter the QPainter to use
     *
     * \a target the target rectangle on the paint device
     *
     * \a source the portion rectangle of the source image
     */
    Q_INVOKABLE void paintFrame(QPainter *painter, const QRectF &target, const QRectF &source = QRectF());

    /*!
     * \brief This method paints the loaded SVG with the elements that
     * represents the border.
     *
     * This is an overloaded member provided for convenience
     *
     * \a painter the QPainter to use
     *
     * \a pos where to paint the svg
     */
    Q_INVOKABLE void paintFrame(QPainter *painter, const QPointF &pos = QPointF(0, 0));

    /*!
     * \brief This method returns the prefix that is actually being used
     * (including a '-' at the end if not empty).
     *
     * \sa prefix()
     */
    QString actualPrefix() const;

    /*!
     * \brief This method returns whether we are in a transaction of many
     * changes at once.
     *
     * This is used to restrict rebuilding generated graphics for each change
     * made.
     *
     * \since 5.31
     */
    bool isRepaintBlocked() const;

    /*!
     * \brief This method sets whether we should block rebuilding generated
     * graphics for each change made.
     *
     * Setting this to \c true will block rebuilding the generated graphics for
     * each change made and will do these changes in blocks instead.
     *
     * How to use this method:
     * When making several changes at once to the frame properties--such as
     * prefix, enabled borders, and size--set this property to true to avoid
     * regenerating the graphics for each change. Set it to false again after
     * applying all required changes.
     *
     * Note that any change will not be visible in the painted frame while this
     * property is set to true.
     * \since 5.31
     */
    void setRepaintBlocked(bool blocked);

    /*!
     * \brief This method returns the minimum height required to correctly draw
     * this SVG.
     *
     * \since 5.101
     */
    Q_INVOKABLE int minimumDrawingHeight();

    /*!
     * \brief This method returns the minimum width required to correctly draw
     * this SVG.
     *
     * \since 5.101
     */
    Q_INVOKABLE int minimumDrawingWidth();

private:
    // Never call this from an inline function
    void colorOverridesChange();

    FrameSvgPrivate *const d;
    friend class FrameData;
    friend class Svg;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(FrameSvg::EnabledBorders)

} // KSvg namespace

#endif // multiple inclusion guard
