/***************************************************************************
 *   Copyright 2010 Marco Martin <mart@kde.org>                            *
 *   Copyright 2014 David Edmundson <davidedmundson@kde.org>               *
 *                                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA .        *
 ***************************************************************************/
#ifndef FRAMESVGITEM_P
#define FRAMESVGITEM_P

#include <QQuickItem>

#include <Plasma/FrameSvg>

#include "units.h"

namespace Plasma
{

class FrameSvg;

/**
 * @class FrameSvgItemMargins
 *
 * @short The sizes of a frame's margins.
 */
class FrameSvgItemMargins : public QObject
{
    Q_OBJECT

    /**
     * Width in pixels of the left margin.
     */
    Q_PROPERTY(qreal left READ left NOTIFY marginsChanged)

    /**
     * Height in pixels of the top margin.
     */
    Q_PROPERTY(qreal top READ top NOTIFY marginsChanged)

    /**
     * Width in pixels of the right margin.
     */
    Q_PROPERTY(qreal right READ right NOTIFY marginsChanged)

    /**
     * Height in pixels of the bottom margin.
     */
    Q_PROPERTY(qreal bottom READ bottom NOTIFY marginsChanged)

    /**
     * Width in pixels of the left and right margins combined.
     */
    Q_PROPERTY(qreal horizontal READ horizontal NOTIFY marginsChanged)

    /**
     * Height in pixels of the top and bottom margins combined.
     */
    Q_PROPERTY(qreal vertical READ vertical NOTIFY marginsChanged)


public:
    FrameSvgItemMargins(Plasma::FrameSvg *frameSvg, QObject *parent = 0);

    qreal left() const;
    qreal top() const;
    qreal right() const;
    qreal bottom() const;
    qreal horizontal() const;
    qreal vertical() const;

    void setFixed(bool fixed);
    bool isFixed() const;

public Q_SLOTS:
    void update();

Q_SIGNALS:
    void marginsChanged();

private:
    FrameSvg *m_frameSvg;
    bool m_fixed;
};


/**
 * @class FrameSvgItem
 *
 * @short Provides an SVG with borders.
 *
 * It is exposed as org.kde.plasma.core.FrameSvgItem
 */
class FrameSvgItem : public QQuickItem
{
    Q_OBJECT

    /**
     * Theme relative path of the svg, like "widgets/background"
     */
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)

    /**
     * prefix for the 9 piece svg, like "pushed" or "normal" for the button
     * see http://techbase.kde.org/Development/Tutorials/Plasma/ThemeDetails
     * for a list of paths and prefixes
     */
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)

    /**
     * The margins of the frame, read only
     * @see FrameSvgItemMargins
     */
    Q_PROPERTY(QObject *margins READ margins CONSTANT)

    /**
     * The margins of the frame, regardless if they are enabled or not
     * read only
     * @see FrameSvgItemMargins
     */
    Q_PROPERTY(QObject *fixedMargins READ fixedMargins CONSTANT)

    Q_FLAGS(Plasma::FrameSvg::EnabledBorders)
    /**
     * The borders that will be rendered, it's a flag combination of:
     *  NoBorder
     *  TopBorder
     *  BottomBorder
     *  LeftBorder
     *  RightBorder
     */
    Q_PROPERTY(Plasma::FrameSvg::EnabledBorders enabledBorders READ enabledBorders WRITE setEnabledBorders NOTIFY enabledBordersChanged)

    /**
     * Holds whether the current svg is present in the current theme and NO fallback is involved
     */
    Q_PROPERTY(bool fromCurrentTheme READ fromCurrentTheme NOTIFY fromCurrentThemeChanged)

    /**
     * Set a color group for the FrameSvgItem.
     * if the Svg uses stylesheets and has elements
     * that are eithe TextColor or BackgroundColor class,
     * make them use ButtonTextColor/ButtonBackgroundColor
     * or ViewTextColor/ViewBackgroundColor, ComplementaryTextColor etc.
     */
    Q_PROPERTY(Plasma::Theme::ColorGroup colorGroup READ colorGroup WRITE setColorGroup NOTIFY colorGroupChanged)

public:
    /**
     * @return true if the svg has the necessary elements with the given prefix
     * to draw a frame
     * @param prefix the given prefix we want to check if drawable
     */
    Q_INVOKABLE bool hasElementPrefix(const QString &prefix) const;

    /// @cond INTERNAL_DOCS
    FrameSvgItem(QQuickItem *parent = 0);
    ~FrameSvgItem();

    void setImagePath(const QString &path);
    QString imagePath() const;

    void setPrefix(const QString &prefix);
    QString prefix() const;

    void setEnabledBorders(const Plasma::FrameSvg::EnabledBorders borders);
    Plasma::FrameSvg::EnabledBorders enabledBorders() const;

    FrameSvgItemMargins *margins() const;
    FrameSvgItemMargins *fixedMargins() const;

    void setColorGroup(Plasma::Theme::ColorGroup group);
    Plasma::Theme::ColorGroup colorGroup() const;

    bool fromCurrentTheme() const;

    void geometryChanged(const QRectF &newGeometry,
                         const QRectF &oldGeometry) Q_DECL_OVERRIDE;

    /**
     * Only to be used from inside this library, is not intended to be invokable
     */
    Plasma::FrameSvg *frameSvg() const;

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) Q_DECL_OVERRIDE;



protected:
    void componentComplete() Q_DECL_OVERRIDE;

/// @endcond

Q_SIGNALS:
    void imagePathChanged();
    void prefixChanged();
    void enabledBordersChanged();
    void fromCurrentThemeChanged();
    void colorGroupChanged();
    void repaintNeeded();

private Q_SLOTS:
    void doUpdate();
    void updateDevicePixelRatio();

private:
    Plasma::FrameSvg *m_frameSvg;
    FrameSvgItemMargins *m_margins;
    FrameSvgItemMargins *m_fixedMargins;
    QString m_prefix;
    Units m_units;
    bool m_textureChanged;
    bool m_sizeChanged;
    bool m_fastPath;
};

}

#endif
