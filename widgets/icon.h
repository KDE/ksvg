/*
*   Copyright (C) 2007 by Siraj Razick <siraj@kde.org>
*   Copyright (C) 2007 by Riccardo Iaconelli <riccardo@kde.org>
*   Copyright (C) 2007 by Matt Broadstone <mbroadst@gmail.com>
*   Copyright 2008 by Alexis Ménard <darktears31@gmail.com>
*
*   This program is free software; you can redistribute it and/or modify
*   it under the terms of the GNU Library General Public License as
*   published by the Free Software Foundation; either version 2, or
*   (at your option) any later version.
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

#ifndef ICON_H
#define ICON_H

#include <QtCore/QObject>
#include <QtGui/QGraphicsTextItem>
#include <QtGui/QIcon>
#include <QtGui/QGraphicsWidget>

#include <plasma/dataengine.h>
#include <plasma/animator.h>
#include <plasma/plasma_export.h>

class QAction;

/**
 * This class provides a generic Icon for the Plasma desktop. An icon, in this
 * sense, is not restricted to just an image, but can also contain text. Currently,
 * the Icon class is primarily used for desktop items, but is designed to be used
 * anywhere an icon is needed in an applet.
 *
 * @author Siraj Razick <siraj@kde.org>
 * @author Matt Broadstone <mbroadst@gmail.com>
 */
namespace Plasma
{

class PLASMA_EXPORT Icon : public QGraphicsWidget
{
    Q_OBJECT
    Q_PROPERTY( QString text READ text WRITE setText )
    Q_PROPERTY( QString infoText READ infoText WRITE setInfoText )
    Q_PROPERTY( QIcon icon READ icon WRITE setIcon )
    Q_PROPERTY( QSizeF iconSize READ iconSize )
    Q_PROPERTY( QString svg WRITE setSvg )
public:
    /**
    * Creates a new Plasma::Icon.
    * @param parent the QGraphicsItem this icon is parented to.
    */
    explicit Icon(QGraphicsItem *parent = 0);

    /**
    * Convenience constructor to create a Plasma::Icon with text.
    * @param text the text that will be displayed with this icon.
    * @param parent the QGraphicsItem this icon is parented to.
    */
    explicit Icon(const QString &text, QGraphicsItem *parent = 0);

    /**
    * Creates a new Plasma::Icon with text and an icon.
    * @param icon the icon that will be displayed with this icon.
    * @param text the text that will be displayed with this icon.
    * @param parent The QGraphicsItem this icon is parented to.
    */
    Icon(const QIcon &icon, const QString &text, QGraphicsItem *parent = 0);

    /**
    * Destroys this Plasma::Icon.
    */
    virtual ~Icon();

    /**
    * Returns the text associated with this icon.
    */
    QString text() const;

    /**
    * Sets the text associated with this icon.
    * @param text the text to associate with this icon.
    */
    void setText(const QString &text);

    /**
    * Convenience method to set the svg image to use when given the filepath and name of svg.
    * @param svgFilePath the svg filepath including name of the svg.
    * @param svgIconElement the svg element to use when displaying the svg. Defaults to all of them.
    */
    void setSvg(const QString &svgFilePath, const QString &svgIconElement = QString());

    /**
    * Returns the meta text associated with this icon.
    */
    QString infoText() const;

    /**
    * Sets the additional information to be displayed by
    * this icon.
    * @param text additional meta text associated with this icon.
    */
    void setInfoText(const QString &text);

    /**
    * @return the icon associated with this icon.
    */
    QIcon icon() const;

    /**
    * Sets the graphical icon for this Plasma::Icon.
    * @param icon the KIcon to associate with this icon.
    */
    void setIcon(const QIcon& icon);

    /**
    * Convenience method to set the icon of this Plasma::Icon
    * using a QString path to the icon.
    * @param icon the path to the icon to associate with this Plasma::Icon.
    */
    Q_INVOKABLE void setIcon(const QString& icon);

    /**
    * @return the size of this Plasma::Icon's graphical icon.
    */
    QSizeF iconSize() const;

    /**
    * Plasma::Icon allows the user to specify a number of actions
    * (current four) to be displayed around the widget. This method
    * allows for a created QAction (not a KAction!) to be added to 
    * the Plasma::Icon.
    * @param action the QAction to associate with this icon.
    */
    void addAction(QAction* action);

    /**
    * let set the orientation of the icon
    * Qt::Vertical: text under the icon
    * Qt::Horizontal text at a side of the icon depending
    * by the direction of the language
    * @param orientation the orientation we want
    */
    void setOrientation(Qt::Orientation orientation);

    /**
    * inverts the layout of the icons if the orientation is horizontal,
    * normally we get icon on the left with left-to-right languages
    * @param invert if we want to invert the layout of icons
    */
    void invertLayout(bool invert);

    /**
    * @return if the layout of the icons should appear inverted or not
    */
    bool invertedLayout() const;

    /**
    * @return optimal size given a size for the icon
    * @param  iconWidth desired width of the icon
    */
    QSizeF sizeFromIconSize(const qreal iconWidth) const;

    /**
    * @return the number of lines allowed to display
    */
    int numDisplayLines();

    /**
    * @param numLines the number of lines to show in the display.
    */
    void setNumDisplayLines(int numLines);

    /**
     * Sets whether or not to draw a background area for the icon
     *
     * @arg draw true if a background should be drawn or not
     */
    void setDrawBackground(bool draw);

    /**
     * @return true if a background area is to be drawn for the icon
     */
    bool drawBackground() const;

    /**
     * reimplemented from QGraphicsItem
     */
    QPainterPath shape() const;

public Q_SLOTS:
    /**
    * Sets the appearance of the icon to pressed or restores the appearance
    * to normal. This does not simulate a mouse button press.
    * @param pressed whether to appear as pressed (true) or as normal (false)
    */
    void setPressed(bool pressed = true);

    /**
    * Shortcut for setPressed(false)
    */
    void setUnpressed();

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = 0);

Q_SIGNALS:
    /**
    * Indicates when the icon has been pressed.
    */
    void pressed(bool down);

    /**
    * Indicates when the icon has been clicked.
    */
    void clicked();

    /**
    * Indicates when the icon has been double-clicked
    */
    void doubleClicked();

    /**
    * Indicates when the icon has been activated following the single
    * or doubleclick settings
    */
    void activated();

protected:
    bool isDown();
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event);

    void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

public:
    class Private;

    /**
    * @internal
    **/
    void drawActionButtonBase(QPainter* painter, const QSize &size, int element);

private:
    void init();
    void layoutIcons(const QStyleOptionGraphicsItem *option);
    void hoverEffect(bool);

    Private * const d;

private Q_SLOTS:
    void actionDestroyed(QObject* obj);
    void readColors();
    void hoverAnimationUpdate(qreal progress);

};

} // namespace Plasma


/*
    // Add these to UrlIcon
    void setUrl(const KUrl& url);
    KUrl url() const;
*/

#endif
