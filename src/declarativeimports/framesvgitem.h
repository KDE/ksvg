/*
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2014 David Edmundson <davidedmundson@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef FRAMESVGITEM_P
#define FRAMESVGITEM_P

#include <QQmlParserStatus>
#include <QQuickItem>

#include <KSvg/FrameSvg>

#include <qqmlregistration.h>

namespace Kirigami
{
namespace Platform
{
class PlatformTheme;
}
};

namespace KSvg
{
class FrameSvg;

class FrameSvgItemMargins : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("FrameSvgItemMargins are read-only properties of FrameSvgItem")

    Q_PROPERTY(qreal left READ left NOTIFY marginsChanged)
    Q_PROPERTY(qreal top READ top NOTIFY marginsChanged)
    Q_PROPERTY(qreal right READ right NOTIFY marginsChanged)
    Q_PROPERTY(qreal bottom READ bottom NOTIFY marginsChanged)
    Q_PROPERTY(qreal horizontal READ horizontal NOTIFY marginsChanged)
    Q_PROPERTY(qreal vertical READ vertical NOTIFY marginsChanged)

public:
    FrameSvgItemMargins(KSvg::FrameSvg *frameSvg, QObject *parent = nullptr);

    qreal left() const;
    qreal top() const;
    qreal right() const;
    qreal bottom() const;
    qreal horizontal() const;
    qreal vertical() const;

    /// returns a vector with left, top, right, bottom
    QList<qreal> margins() const;

    void setFixed(bool fixed);
    bool isFixed() const;

    void setInset(bool inset);
    bool isInset() const;

public Q_SLOTS:
    void update();

Q_SIGNALS:
    void marginsChanged();

private:
    FrameSvg *m_frameSvg;
    bool m_fixed;
    bool m_inset;
};

/*!
 * \qmltype FrameSvgItem
 * \inqmlmodule org.kde.ksvg
 *
 * \brief An SVG Item with borders.
 */
class FrameSvgItem : public QQuickItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_INTERFACES(QQmlParserStatus)

    /*!
     * \brief This property specifies the relative path of the SVG in the theme.
     *
     * Example: "widgets/background"
     *
     * \qmlproperty string FrameSvgItem::imagePath
     */
    Q_PROPERTY(QString imagePath READ imagePath WRITE setImagePath NOTIFY imagePathChanged)

    /*!
     * This property holds the prefix for the SVG.
     * prefix for the 9-piece SVG, like "pushed" or "normal" for a button.
     * see https://techbase.kde.org/Development/Tutorials/Plasma5/ThemeDetails
     * for a list of paths and prefixes
     * It can also be an array of strings, specifying a fallback chain in case
     * the first element isn't found in the theme, eg ["toolbutton-normal", "normal"]
     * so it's easy to keep backwards compatibility with old themes
     * (Note: fallback chain is supported only \since 5.32)
     *
     * \qmlproperty variant FrameSvgItem::prefix
     */
    Q_PROPERTY(QVariant prefix READ prefix WRITE setPrefix NOTIFY prefixChanged)

    /*!
     * \brief This property holds the actual prefix that was used, if a fallback
     * chain array was set as "prefix".
     *
     * \since 5.34
     * \qmlproperty string FrameSvgItem::usedPrefix
     */
    Q_PROPERTY(QString usedPrefix READ usedPrefix NOTIFY usedPrefixChanged)

    /*!
     * \qmlproperty qreal FrameSvgItem::margins.left
     * \qmlproperty qreal FrameSvgItem::margins.top
     * \qmlproperty qreal FrameSvgItem::margins.right
     * \qmlproperty qreal FrameSvgItem::margins.bottom
     * \qmlproperty qreal FrameSvgItem::margins.horizontal
     * \qmlproperty qreal FrameSvgItem::margins.vertical
     *
     * \brief This property holds the frame's margins.
     */
    Q_PROPERTY(KSvg::FrameSvgItemMargins *margins READ margins CONSTANT)

    /*!
     * \qmlproperty qreal FrameSvgItem::fixedMargins.left
     * \qmlproperty qreal FrameSvgItem::fixedMargins.top
     * \qmlproperty qreal FrameSvgItem::fixedMargins.right
     * \qmlproperty qreal FrameSvgItem::fixedMargins.bottom
     * \qmlproperty qreal FrameSvgItem::fixedMargins.horizontal
     * \qmlproperty qreal FrameSvgItem::fixedMargins.vertical
     *
     * \brief This property holds the fixed margins of the frame which are used
     * regardless of any margins being enabled or not.
     */
    Q_PROPERTY(KSvg::FrameSvgItemMargins *fixedMargins READ fixedMargins CONSTANT)

    /*!
     * \qmlproperty qreal FrameSvgItem::inset.left
     * \qmlproperty qreal FrameSvgItem::inset.top
     * \qmlproperty qreal FrameSvgItem::inset.right
     * \qmlproperty qreal FrameSvgItem::inset.bottom
     * \qmlproperty qreal FrameSvgItem::inset.horizontal
     * \qmlproperty qreal FrameSvgItem::inset.vertical
     *
     * \brief This property holds the frame's inset.
     *
     * \since 5.77
     */
    Q_PROPERTY(KSvg::FrameSvgItemMargins *inset READ inset CONSTANT)

    /*!
     * \brief This property specifies which borders are shown.
     * \sa KSvg::FrameSvg::EnabledBorder
     * \qmlproperty flags<KSvg::FrameSvg::EnabledBorder> FrameSvgItem::enabledBorders
     */
    Q_PROPERTY(KSvg::FrameSvg::EnabledBorders enabledBorders READ enabledBorders WRITE setEnabledBorders NOTIFY enabledBordersChanged)

    /*!
     * \brief This property holds whether the current SVG is present in
     * the currently-used theme and no fallback is involved.
     *
     * \qmlproperty bool FrameSvgItem::fromCurrentImageSet
     */
    Q_PROPERTY(bool fromCurrentImageSet READ fromCurrentImageSet NOTIFY fromCurrentImageSetChanged)

    /*!
     * \brief This property specifies the SVG's status.
     *
     * Depending on the specified status, the SVG will use different colors:
     * * Normal: text's color is textColor, and background color is
     * backgroundColor.
     * * Selected: text color becomes highlightedText and background color is
     * changed to highlightColor.
     *
     * \sa Kirigami::PlatformTheme
     * \sa KSvg::Svg::status
     * \since 5.23
     * \qmlproperty enum<KSvg::Svg::Status> FrameSvgItem::status
     */
    Q_PROPERTY(KSvg::Svg::Status status READ status WRITE setStatus NOTIFY statusChanged)

    /*!
     * \brief This property holds the mask that contains the SVG's painted areas.
     * \since 5.58
     * \qmlproperty QRegion FrameSvgItem::mask
     */
    Q_PROPERTY(QRegion mask READ mask NOTIFY maskChanged)

    /*!
     * \brief This property holds the minimum height required to correctly draw
     * this SVG.
     *
     * \since 5.101
     * \qmlproperty int FrameSvgItem::minimumDrawingHeight
     */
    Q_PROPERTY(int minimumDrawingHeight READ minimumDrawingHeight NOTIFY repaintNeeded)

    /*!
     * \brief This property holds the minimum width required to correctly draw
     * this SVG.
     *
     * \since 5.101
     * \qmlproperty int FrameSvgItem::minimumDrawingWidth
     */
    Q_PROPERTY(int minimumDrawingWidth READ minimumDrawingWidth NOTIFY repaintNeeded)

public:
    /*!
     * \qmlmethod bool FrameSvgItem::hasElementPrefix(string prefix)
     *
     * Returns whether the svg has the necessary elements with the given prefix
     * to draw a frame.
     *
     * \a prefix the given prefix we want to check if drawable
     */
    Q_INVOKABLE bool hasElementPrefix(const QString &prefix) const;

    /*!
     * \qmlmethod bool FrameSvgItem::hasElement(string elementName)
     *
     * Returns whether the SVG includes the given element.
     *
     * This is a convenience function that forwards to hasElement().
     *
     * \sa KSvg::Svg::hasElement()
     */
    Q_INVOKABLE bool hasElement(const QString &elementName) const;

    FrameSvgItem(QQuickItem *parent = nullptr);
    ~FrameSvgItem() override;

    void setImagePath(const QString &path);
    QString imagePath() const;

    void setPrefix(const QVariant &prefix);
    QVariant prefix() const;

    QString usedPrefix() const;

    void setEnabledBorders(const KSvg::FrameSvg::EnabledBorders borders);
    KSvg::FrameSvg::EnabledBorders enabledBorders() const;

    void setColorSet(KSvg::Svg::ColorSet colorSet);
    KSvg::Svg::ColorSet colorSet() const;

    FrameSvgItemMargins *margins();
    FrameSvgItemMargins *fixedMargins();
    FrameSvgItemMargins *inset();

    bool fromCurrentImageSet() const;

    void setStatus(KSvg::Svg::Status status);
    KSvg::Svg::Status status() const;
    int minimumDrawingHeight() const;
    int minimumDrawingWidth() const;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

    QRegion mask() const;

    /*
     * Only to be used from inside this library, is not intended to be invokable
     */
    KSvg::FrameSvg *frameSvg() const;

    QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *) override;

    void itemChange(QQuickItem::ItemChange change, const QQuickItem::ItemChangeData &data) override;

protected:
    void classBegin() override;
    void componentComplete() override;

Q_SIGNALS:
    void imagePathChanged();
    void prefixChanged();
    void enabledBordersChanged();
    void fromCurrentImageSetChanged();
    void repaintNeeded();
    void statusChanged();
    void usedPrefixChanged();
    void maskChanged();

private Q_SLOTS:
    void doUpdate();

private:
    void updateDevicePixelRatio();
    void applyPrefixes();

    KSvg::FrameSvg *m_frameSvg;
    Kirigami::Platform::PlatformTheme *m_kirigamiTheme;
    FrameSvgItemMargins *m_margins;
    FrameSvgItemMargins *m_fixedMargins;
    FrameSvgItemMargins *m_insetMargins;
    // logged margins to check for changes
    QList<qreal> m_oldMargins;
    QList<qreal> m_oldFixedMargins;
    QList<qreal> m_oldInsetMargins;
    QStringList m_prefixes;
    bool m_textureChanged;
    bool m_sizeChanged;
    bool m_fastPath;
};

}

#endif
