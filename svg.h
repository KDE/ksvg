/*
 *   Copyright (C) 2006 Aaron Seigo <aseigo@kde.org>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU Library General Public License version 2 as
 *   published by the Free Software Foundation
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details
 *
 *   You should have received a copy of the GNU Library General Public
 *   License along with this program; if not, write to the
 *   Free Software Foundation, Inc.,
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef PLASMA_SVG_H
#define PLASMA_SVG_H

#include <QtCore/QObject>

#include <plasma/plasma_export.h>

class QPainter;
class QPoint;
class QPointF;
class QRect;
class QRectF;
class QSize;
class QSizeF;

namespace Plasma
{

/**
 * @brief A theme aware image-centric SVG class
 *
 * Plasma::Svg provides a class for rendering SVG images to a QPainter in a
 * convenient manner. Unless an absolute path to a file is provided, it loads
 * the SVG document using Plasma::Theme. It also provides a number of internal
 * optimizations to help lower the cost of painting SVGs, such as caching.
 **/
class PLASMA_EXPORT Svg : public QObject
{
    Q_OBJECT
    Q_ENUMS( ContentType )
    Q_PROPERTY( QSize size READ size )
    Q_PROPERTY( ContentType contentType READ contentType WRITE setContentType )

    public:
        /**
         * Describes what is in the Svg document to be loaded. The important
         * distinction is whether the Svg is a single image (or group of
         * elements) that are meant to be viewed together, such as a clock face
         * and hands, or a set of images that are meant to be displayed
         * individually, such as a deck of cards.
         */
        enum ContentType { SingleImage = 0 /**< A set of elements that together
                                                make an image. Elements may be
                                                drawn separately to accomplish
                                                this. This is the default. */,
                           ImageSet /**< A set of elements, each of which
                                         constitutes a whole image. Each
                                         element will therefore be rendered
                                         to the set size of the Svg object */
                         };

        /**
         * Constructs an SVG object that implicitly shares and caches rendering
         * As opposed to QSvgRenderer, which this class uses internally,
         * Plasma::Svg represents an image generated from an SVG. As such, it
         * has a related size and transform matrix (the latter being provided
         * by the painter used to paint the image).
         *
         * The size is initialized to be the SVG's native size.
         *
         * @arg imagePath the image to show. If a relative path is passed, then
         *      Plasma::Theme is used to load the SVG.
         * @arg parent options QObject to parent this to
         *
         * @related Plasma::Theme
         */
        explicit Svg(const QString& imagePath, QObject* parent = 0);
        ~Svg();


        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg point the position to start drawing; the entire svg will be
         *      drawn starting at this point.
         */
        Q_INVOKABLE void paint(QPainter* painter, const QPointF& point,
                               const QString& elementID = QString());

        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg x the horizontal coordinate to start painting from
         * @arg y the vertical coordinate to start painting from
         */
        Q_INVOKABLE void paint(QPainter* painter, int x, int y,
                               const QString& elementID = QString());

        /**
         * Paints the SVG represented by this object
         * @arg painter the QPainter to use
         * @arg rect the rect to draw into; if small than the current size
         *           of the 
         *      drawn starting at this point.
         */
        Q_INVOKABLE void paint(QPainter* painter, const QRectF& rect,
                               const QString& elementID = QString());

        /**
         * Resizes the rendered image. Rendering will actually take place on
         * the next call to paint.
         * @arg width the new width
         * @arg height the new height
         **/
        Q_INVOKABLE void resize( int width, int height );

        /**
         * Resizes the rendered image. Rendering will actually take place on
         * the next call to paint.
         * @arg size the new size of the image
         **/
        Q_INVOKABLE void resize( const QSizeF& size );

        /**
         * Resizes the rendered image to the natural size of the SVG.
         * Rendering will actually take place on the next call to paint.
         **/
        Q_INVOKABLE void resize();

        /**
         * Size of a given element
         * @arg elementId the id of the element to check
         * @return the current size of a given element, given the current size of the Svg
         **/
        Q_INVOKABLE QSize elementSize( const QString& elementId ) const;

        /**
         * The bounding rect of a given element
         * @arg elementId the id of the element to check
         * @return the current rect of a given element, given the current size of the Svg
         **/
        Q_INVOKABLE QRect elementRect(const QString& elementId) const;

        /**
         * Check when an element exists in the loaded Svg
         * @arg elementId the id of the element to check
         * @return true if the element is defined in the Svg, otherwise false
         **/
        Q_INVOKABLE bool elementExists( const QString& elementId ) const;

        /**
         * @return true if the SVG file exists and the document is valid,
         *         otherwise false. This method can be expensive as it
         *         causes disk access.
         **/
        Q_INVOKABLE bool isValid() const;

        /**
         * Currently set size of the SVG
         * @return the current size of a given element
         **/
        QSize size() const;

        /**
         * Sets what sort of content is in the Svg.
         * @see ContentType
         * @arg contents whether the Svg is a single image or a set of images
         */
        void setContentType(ContentType contentType);

        /**
         * Returns the content type of the Svg
         * @see SetContentType
         * @arg contents whether the Svg is a single image or a set of images
         */
        ContentType contentType();

    Q_SIGNALS:
        void repaintNeeded();

    private Q_SLOTS:
        void themeChanged();

    private:
        class Private;
        Private* const d;
};

} // Plasma namespace

#endif // multiple inclusion guard

