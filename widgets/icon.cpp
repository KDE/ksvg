/*
 *   Copyright 2007 by Aaron Seigo <aseigo@kde.org>
 *   Copyright 2007 by Riccardo Iaconelli <riccardo@kde.org>
 *   Copyright 2007 by Matt Broadstone <mbroadst@gmail.com>
 *   Copyright 2006-2007 Fredrik Höglund <fredrik@kde.org>
 *   Copyright 2007 by Marco Martin <notmart@gmail.com>
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

#include "icon.h"
#include "icon_p.h"

#include <QAction>
#include <QApplication>
#include <QPainter>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsView>
#include <QStyleOptionGraphicsItem>
#include <QTextLayout>

//#define BACKINGSTORE_BLUR_HACK

#ifdef BACKINGSTORE_BLUR_HACK
#include <private/qwindowsurface_p.h>
#include "effects/blur.cpp"
#endif

#include <KGlobalSettings>
#include <KIconEffect>
#include <KIconLoader>
#include <KIcon>
#include <KUrl>
#include <KRun>
#include <KMimeType>
#include <KDebug>
#include <KColorScheme>

#include <plasma/paintutils.h>
#include <plasma/theme.h>

#include "animator.h"
#include "svg.h"

namespace Plasma
{

IconPrivate::IconPrivate(Icon *i)
    : q(i),
      iconSvg(0),
      iconSize(48, 48),
      states(IconPrivate::NoState),
      orientation(Qt::Vertical),
      numDisplayLines(2),
      invertLayout(false),
      drawBg(false),
      action(0),
      activeMargins(0)
{
    m_hoverAnimId = -1;
    m_hoverAlpha = 20/255;
}

IconPrivate::~IconPrivate()
{
    qDeleteAll(cornerActions);
    delete iconSvg;
}

void Icon::readColors() 
{
    d->textColor = Plasma::Theme::defaultTheme()->color(Theme::TextColor);
    d->shadowColor = Plasma::Theme::defaultTheme()->color(Theme::BackgroundColor);

}

IconAction::IconAction(Icon* icon, QAction *action)
    : m_icon(icon),
      m_action(action),
      m_hovered(false),
      m_pressed(false),
      m_visible(false),
      m_animationId(-1)
{
}

void IconAction::show()
{
    if (m_animationId) {
        Animator::self()->stopElementAnimation(m_animationId);
    }

    rebuildPixmap();

    m_animationId = Animator::self()->animateElement(m_icon, Animator::AppearAnimation);
    Animator::self()->setInitialPixmap(m_animationId, m_pixmap);
    m_visible = true;
}

void IconAction::hide()
{
    if (m_animationId) {
        Animator::self()->stopElementAnimation(m_animationId);
    }

    rebuildPixmap();

    m_animationId = Animator::self()->animateElement(m_icon, Animator::DisappearAnimation);
    Animator::self()->setInitialPixmap(m_animationId, m_pixmap);
    m_visible = false;
}

bool IconAction::isVisible() const
{
    return m_visible;
}

bool IconAction::isPressed() const
{
    return m_pressed;
}

bool IconAction::isHovered() const
{
    return m_hovered;
}

void IconAction::setSelected(bool selected)
{
    m_selected = selected;
}

bool IconAction::isSelected() const
{
    return m_selected;
}

void IconAction::setRect(const QRectF &rect)
{
    m_rect = rect;
}

QRectF IconAction::rect() const
{
    return m_rect;
}

void IconAction::rebuildPixmap()
{
    // Determine proper QIcon mode based on selection status
    QIcon::Mode mode = QIcon::Normal;
    if (m_selected) {
        mode = QIcon::Selected;
    }

    // Draw everything
    m_pixmap = QPixmap(26, 26);
    m_pixmap.fill(Qt::transparent);

    int element = IconPrivate::Minibutton;
    if (m_pressed) {
        element = IconPrivate::MinibuttonPressed;
    } else if (m_hovered) {
        element = IconPrivate::MinibuttonHover;
    }

    QPainter painter(&m_pixmap);
    m_icon->drawActionButtonBase(&painter, m_pixmap.size(), element);
    m_action->icon().paint(&painter, 2, 2, 22, 22, Qt::AlignCenter, mode);
}

bool IconAction::event(QEvent::Type type, const QPointF &pos)
{
    if (m_icon->size().width() < m_rect.width()*2.0 ||
        m_icon->size().height() < m_rect.height()*2.0) {
        return false;
    }

    switch (type) {
    case QEvent::GraphicsSceneMousePress: {
        setSelected(m_rect.contains(pos));
        return isSelected();
        }
        break;

    case QEvent::GraphicsSceneMouseMove: {
        bool wasSelected = isSelected();
        bool active = m_rect.contains(pos);
        setSelected(wasSelected && active);
        return (wasSelected != isSelected()) || active;
        }
        break;

    case QEvent::GraphicsSceneMouseRelease: {
        // kDebug() << "IconAction::event got a QEvent::MouseButtonRelease, " << isSelected();
        bool wasSelected = isSelected();
        setSelected(false);
        if (wasSelected) {
            m_action->trigger();
        }

        return wasSelected;
        }
        break;

    case QEvent::GraphicsSceneHoverEnter:
        m_pressed = false;
        m_hovered = true;
        break;

    case QEvent::GraphicsSceneHoverLeave:
        m_pressed = false;
        m_hovered = false;
        break;

    default:
            break;
    }

    return false;
}

int IconAction::animationId() const
{
    return m_animationId;
}

QAction* IconAction::action() const
{
    return m_action;
}

void IconAction::paint(QPainter *painter) const
{
    if (m_icon->size().width() < m_rect.width()*2.0 ||
        m_icon->size().height() < m_rect.height()*2.0) {
        return;
    }

    QPixmap animPixmap = Animator::self()->currentPixmap(m_animationId);

    if (m_visible && animPixmap.isNull()) {
        painter->drawPixmap(m_rect.toRect(), m_pixmap);
    } else {
        painter->drawPixmap(m_rect.toRect(), animPixmap);
    }
}

Icon::Icon(QGraphicsItem *parent)
    : QGraphicsWidget(parent),
      d(new IconPrivate(this))
{
    init();
}

Icon::Icon(const QString &text, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
      d(new IconPrivate(this))
{
    init();
    setText(text);
}

Icon::Icon(const QIcon &icon, const QString &text, QGraphicsItem *parent)
    : QGraphicsWidget(parent),
      d(new IconPrivate(this))
{
    init();
    setText(text);
    setIcon(icon);
}

Icon::~Icon()
{
    delete d;
}

void Icon::init()
{
    readColors();
    connect(Plasma::Theme::defaultTheme(), SIGNAL(themeChanged()), SLOT(readColors()));

    // setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptsHoverEvents(true);

    int focusHMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameHMargin);
    int focusVMargin = QApplication::style()->pixelMetric(QStyle::PM_FocusFrameVMargin);

    // Margins for horizontal mode (list views, tree views, table views)
    d->setHorizontalMargin(IconPrivate::TextMargin, focusHMargin, focusVMargin);
    d->setHorizontalMargin(IconPrivate::IconMargin, focusHMargin, focusVMargin);
    d->setHorizontalMargin(IconPrivate::ItemMargin, 0, 0);

    // Margins for vertical mode (icon views)
    d->setVerticalMargin(IconPrivate::TextMargin, 6, 2);
    d->setVerticalMargin(IconPrivate::IconMargin, focusHMargin, focusVMargin);
    d->setVerticalMargin(IconPrivate::ItemMargin, 0, 0);

    d->setActiveMargins();
    d->currentSize = QSizeF(-1, -1);
    //setDrawStandardBackground(false);
}

void Icon::addIconAction(QAction *action)
{
    int count = d->cornerActions.count();
    if (count > 3) {
        kDebug() << "no more room for more actions!";
    }

    IconAction* iconAction = new IconAction(this, action);
    d->cornerActions.append(iconAction);
    connect(action, SIGNAL(destroyed(QObject*)), this, SLOT(actionDestroyed(QObject*)));

    iconAction->setRect(d->actionRect((IconPrivate::ActionPosition)count));
}

void Icon::setAction(QAction *action)
{
    if (d->action) {
        disconnect(d->action, 0, this, 0);
        disconnect(this, 0, d->action, 0);
    }
    d->action = action;
    if (action) {
        connect(action, SIGNAL(changed()), this, SLOT(syncToAction()));
        connect(this, SIGNAL(clicked()), action, SLOT(trigger()));
        d->syncToAction();
    }
}

QAction* Icon::action() const
{
    return d->action;
}

void Icon::actionDestroyed(QObject* action)
{
    QList<IconAction*>::iterator it = d->cornerActions.begin();

    while (it != d->cornerActions.end()) {
        if ((*it)->action() == action) {
            d->cornerActions.erase(it);
            break;
        }
    }

    update();   // redraw since an action has been deleted.
}

int Icon::numDisplayLines()
{
    return d->numDisplayLines;
}

void Icon::setNumDisplayLines(int numLines)
{
    if(numLines > d->maxDisplayLines) {
        d->numDisplayLines = d->maxDisplayLines;
    } else {
        d->numDisplayLines = numLines;
    }
}

void Icon::setDrawBackground(bool draw)
{
    if (d->drawBg != draw) {
        d->drawBg = draw;
        update();
    }
}

bool Icon::drawBackground() const
{
    return d->drawBg;
}

QPainterPath Icon::shape() const
{
    if (d->currentSize.width() < 1) {
        return QGraphicsItem::shape();
    }

    return PaintUtils::roundedRectangle(QRectF(QPointF(0.0, 0.0), d->currentSize).adjusted(-2, -2, 2, 2), 10.0);
}

QSizeF IconPrivate::displaySizeHint(const QStyleOptionGraphicsItem *option, const qreal width) const
{
    if (text.isEmpty() && infoText.isEmpty()) {
      return QSizeF( .0, .0 );
    }
    QString label = text;
    // const qreal maxWidth = (orientation == Qt::Vertical) ? iconSize.width() + 10 : 32757;
    // NOTE: find a way to use the other layoutText, it currently returns nominal width, when
    //       we actually need the actual width.


    qreal textWidth = width -
                      horizontalMargin[IconPrivate::TextMargin].left -
                      horizontalMargin[IconPrivate::TextMargin].right;

    //allow only five lines of text
    const qreal maxHeight = numDisplayLines*Plasma::Theme::defaultTheme()->fontMetrics().lineSpacing();

    // To compute the nominal size for the label + info, we'll just append
    // the information string to the label
    if (!infoText.isEmpty()) {
        label += QString(QChar::LineSeparator) + infoText;
    }

    QTextLayout layout;
    setLayoutOptions(layout, option);
    QSizeF size = layoutText(layout, option, label, QSizeF(textWidth, maxHeight));

    return addMargin(size, TextMargin);
}

void Icon::layoutIcons(const QStyleOptionGraphicsItem *option)
{
    if (size() == d->currentSize) {
        return;
    }

    d->currentSize = size();
    d->setActiveMargins();

    //calculate icon size based on the available space
    qreal iconWidth;

    if (d->orientation == Qt::Vertical) {
        qreal heightAvail;
        //if there is text resize the icon in order to make room for the text
        if (!d->text.isEmpty() || !d->infoText.isEmpty()) {
            heightAvail = d->currentSize.height() -
                          d->displaySizeHint(option, d->currentSize.width()).height() -
                          d->verticalMargin[IconPrivate::TextMargin].top -
                          d->verticalMargin[IconPrivate::TextMargin].bottom;
            //never make a label higher than half the total height
            heightAvail = qMax(heightAvail, d->currentSize.height()/2);
        } else {
            heightAvail = d->currentSize.height();
        }

        //aspect ratio very "tall"
        if (d->currentSize.width() < heightAvail) {
            iconWidth = d->currentSize.width() -
                        d->horizontalMargin[IconPrivate::IconMargin].left -
                        d->horizontalMargin[IconPrivate::IconMargin].right;
        } else {
            iconWidth = heightAvail -
                        d->verticalMargin[IconPrivate::IconMargin].top -
                        d->verticalMargin[IconPrivate::IconMargin].bottom;
        }
    } else {
        //Horizontal layout
        QFontMetricsF fm(font());

        //if there is text resize the icon in order to make room for the text
        if (d->text.isEmpty() && d->infoText.isEmpty()) {
            // with no text, we just take up the whole geometry
            iconWidth = d->currentSize.width() -
                        d->horizontalMargin[IconPrivate::IconMargin].left -
                        d->horizontalMargin[IconPrivate::IconMargin].right;
        } else {
            iconWidth = d->currentSize.height() -
                        d->verticalMargin[IconPrivate::IconMargin].top -
                        d->verticalMargin[IconPrivate::IconMargin].bottom;
        }
    }

    d->iconSize = QSizeF(iconWidth, iconWidth);

    int count = 0;
    foreach (IconAction* iconAction, d->cornerActions) {
        iconAction->setRect(d->actionRect((IconPrivate::ActionPosition)count));
        ++count;
    }
}

void Icon::setSvg(const QString &svgFilePath, const QString &elementId)
{
    if (!d->iconSvg) {
        d->iconSvg = new Plasma::Svg();
    }

    d->iconSvg->setImagePath(svgFilePath);

    d->iconSvgElement = elementId;
}

void Icon::hoverEffect(bool show)
{
    if (show) {
        d->states |= IconPrivate::HoverState;
    }

    if (d->m_hoverAnimId == -1 && !d->drawBg) {
        // we do this only when we don't do the anim, because
        // this gets set at animation end when we are animating
        if (!show) {
            d->states &= ~IconPrivate::HoverState;
        }

        return;
    }

    d->m_fadeIn = show;
    const int FadeInDuration = 150;

    if (d->m_hoverAnimId != -1) {
        Animator::self()->stopCustomAnimation(d->m_hoverAnimId);
    }
    d->m_hoverAnimId = Animator::self()->customAnimation(40 / (1000 / FadeInDuration), FadeInDuration,
                                                         Animator::EaseOutCurve, this,
                                                         "hoverAnimationUpdate");
}

void Icon::hoverAnimationUpdate(qreal progress) 
{
    if (d->m_fadeIn) {
        d->m_hoverAlpha = progress;
    } else {
        // If we mouse leaves before the fade in is done, fade out from where we were,
        // not from fully faded in
        qreal new_alpha = d->m_fadeIn ? progress : 1 - progress;
        d->m_hoverAlpha = qMin(new_alpha, d->m_hoverAlpha);
    }
    if (progress == 1) {
        d->m_hoverAnimId = -1;
    }
    if (!d->m_fadeIn && progress == 1 ) {
        d->states &= ~IconPrivate::HoverState;
    }
    update();
}

void IconPrivate::drawBackground(QPainter *painter, IconState state)
{
    if (!drawBg) {
        return;
    }

    bool darkShadow = shadowColor.value() < 128;
    QColor shadow = shadowColor;
    QColor border = textColor;

    switch (state) {
        case IconPrivate::HoverState:
            shadow.setHsv(shadow.hue(),
                          shadow.saturation(),
                          shadow.value() + (int)(darkShadow ? 50 * m_hoverAlpha: -50 * m_hoverAlpha),
                          200 + (int)m_hoverAlpha * 55); // opacity
            break;
        case IconPrivate::PressedState:
            shadow.setHsv(shadow.hue(),
                          shadow.saturation(),
                          shadow.value() + (darkShadow?(int)(50*m_hoverAlpha):(int)(-50*m_hoverAlpha)),
                          204); //80% opacity
            break;
        default:
            break;
    }
    
    border.setAlphaF(0.3*m_hoverAlpha);
    shadow.setAlphaF(0.6*m_hoverAlpha);

    painter->save();
    painter->translate(0.5, 0.5);
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setBrush(shadow);
    painter->setPen(QPen(border, 1));
    painter->drawPath(PaintUtils::roundedRectangle(QRectF(QPointF(1, 1), QSize((int)currentSize.width()-2, (int)currentSize.height()-2)), 5.0));
    painter->restore();
}

QPixmap IconPrivate::decoration(const QStyleOptionGraphicsItem *option, bool useHoverEffect)
{
    QPixmap result;

    QIcon::Mode mode   = option->state & QStyle::State_Enabled ? QIcon::Normal : QIcon::Disabled;
    QIcon::State state = option->state & QStyle::State_Open ? QIcon::On : QIcon::Off;

    if (iconSvg) {
        if (iconSvgPixmap.size() != iconSize.toSize()) {
            QImage img(iconSize.toSize(), QImage::Format_ARGB32_Premultiplied);
            {
                img.fill(0);
                QPainter p(&img);
                iconSvg->resize(iconSize);
                iconSvg->paint(&p, img.rect(), iconSvgElement);
            }
            iconSvgPixmap = QPixmap::fromImage(img);
        }
        result = iconSvgPixmap;
    } else {
        const QSize size = icon.actualSize(iconSize.toSize(), mode, state);
        result = icon.pixmap(size, mode, state);
    }

    // We disable the iconeffect here since we cannot get it into sync with
    // the fade animation. TODO: Enable it when animations are switched off
    if (!result.isNull() && useHoverEffect && !drawBg) {
        KIconEffect *effect = KIconLoader::global()->iconEffect();
        // Note that in KIconLoader terminology, active = hover.
        // We're assuming that the icon group is desktop/filemanager, since this
        // is KFileItemDelegate.
        if (effect->hasEffect(KIconLoader::Desktop, KIconLoader::ActiveState)) {
            result = effect->apply(result, KIconLoader::Desktop, KIconLoader::ActiveState);
        }
    }

    return result;
}

QPointF IconPrivate::iconPosition(const QStyleOptionGraphicsItem *option, const QPixmap &pixmap) const
{
    const QRectF itemRect = subtractMargin(option->rect, IconPrivate::ItemMargin);

    // Compute the nominal decoration rectangle
    const QSizeF size = addMargin(iconSize, IconPrivate::IconMargin);

    Qt::LayoutDirection direction = iconDirection(option);

    //alignment depends from orientation and option->direction
    Qt::Alignment alignment;
    if (text.isEmpty() && infoText.isEmpty()) {
        alignment = Qt::AlignCenter;
    }else if (orientation == Qt::Vertical) {
        alignment = Qt::Alignment(Qt::AlignHCenter | Qt::AlignTop);
    //Horizontal
    }else{
        alignment = QStyle::visualAlignment(direction, Qt::Alignment(Qt::AlignLeft | Qt::AlignVCenter));
    }

    const QRect iconRect = QStyle::alignedRect(direction, alignment, size.toSize(), itemRect.toRect());

    // Position the pixmap in the center of the rectangle
    QRect pixmapRect = pixmap.rect();
    pixmapRect.moveCenter(iconRect.center());

    // add a gimmicky margin of 5px to y, TEMP TEMP TEMP
    // pixmapRect = pixmapRect.adjusted(0, 5, 0, 0);

    return QPointF(pixmapRect.topLeft());
}

QRectF IconPrivate::labelRectangle(const QStyleOptionGraphicsItem *option, const QPixmap &icon,
                                    const QString &string) const
{
    Q_UNUSED(string)

    if (icon.isNull()) {
        return option->rect;
    }

    const QSizeF decoSize = addMargin(iconSize, IconPrivate::IconMargin);
    const QRectF itemRect = subtractMargin(option->rect, IconPrivate::ItemMargin);
    QRectF textArea(QPointF(0, 0), itemRect.size());

    if (orientation == Qt::Vertical) {
        textArea.setTop(decoSize.height() + 1);
    } else {
        //Horizontal
       textArea.setLeft(decoSize.width() + 1);
    }

    textArea.translate(itemRect.topLeft());
    return QRectF(QStyle::visualRect(iconDirection(option), option->rect, textArea.toRect()));
}

// Lays the text out in a rectangle no larger than constraints, eliding it as necessary
QSizeF IconPrivate::layoutText(QTextLayout &layout, const QStyleOptionGraphicsItem *option,
                                const QString &text, const QSizeF &constraints) const
{
    const QSizeF size = layoutText(layout, text, constraints.width());

    if (size.width() > constraints.width() || size.height() > constraints.height())
    {
        const QString elided = elidedText(layout, option, constraints);
        return layoutText(layout, elided, constraints.width());
    }

    return size;
}

// Lays the text out in a rectangle no wider than maxWidth
QSizeF IconPrivate::layoutText(QTextLayout &layout, const QString &text, qreal maxWidth) const
{
    QFontMetricsF metrics(layout.font());
    qreal leading     = metrics.leading();
    qreal height      = 0.0;
    qreal widthUsed   = 0.0;
    QTextLine line;

    layout.setText(text);

    layout.beginLayout();

    while ((line = layout.createLine()).isValid())
    {
        line.setLineWidth(maxWidth);
        height += leading;
        line.setPosition(QPointF(0.0, height));
        height += line.height();
        widthUsed = qMax(widthUsed, line.naturalTextWidth());
    }
    layout.endLayout();

    return QSizeF(widthUsed, height);
}

// Elides the text in the layout, by iterating over each line in the layout, eliding
// or word breaking the line if it's wider than the max width, and finally adding an
// ellipses at the end of the last line, if there are more lines than will fit within
// the vertical size constraints.
QString IconPrivate::elidedText(QTextLayout &layout, const QStyleOptionGraphicsItem *option,
                                  const QSizeF &size) const
{
    Q_UNUSED(option)

    QFontMetricsF metrics(layout.font());
    const QString text = layout.text();
    qreal maxWidth       = size.width();
    qreal maxHeight      = size.height();
    qreal height         = 0;

    // Elide each line that has already been laid out in the layout.
    QString elided;
    elided.reserve(text.length());

    for (int i = 0; i < layout.lineCount(); i++)
    {
        QTextLine line = layout.lineAt(i);
        int start  = line.textStart();
        int length = line.textLength();

        height += metrics.leading();
        if (height + line.height() + metrics.lineSpacing() > maxHeight)
        {
            // Unfortunately, if the line ends because of a line separator, elidedText() will be too
            // clever and keep adding lines until it finds one that's too wide.
            if (line.naturalTextWidth() < maxWidth && start+length > 0 && text[start + length - 1] == QChar::LineSeparator)
                elided += text.mid(start, length - 1);
            else
                elided += metrics.elidedText(text.mid(start), Qt::ElideRight, maxWidth);
            break;
        }
        else if (line.naturalTextWidth() > maxWidth)
            elided += metrics.elidedText(text.mid(start, length), Qt::ElideRight, maxWidth);
        else
            elided += text.mid(start, length);

        height += line.height();
    }

    return elided;
}

void IconPrivate::layoutTextItems(const QStyleOptionGraphicsItem *option,
                                    const QPixmap &icon, QTextLayout *labelLayout,
                                    QTextLayout *infoLayout, QRectF *textBoundingRect) const
{
    bool showInformation = false;

    setLayoutOptions(*labelLayout, option);

    QFontMetricsF fm(labelLayout->font());
    const QRectF textArea = labelRectangle(option, icon, text);
    QRectF textRect = subtractMargin(textArea, IconPrivate::TextMargin);

    //kDebug() << this << "text area" << textArea << "text rect" << textRect;
    // Sizes and constraints for the different text parts
    QSizeF maxLabelSize = textRect.size();
    QSizeF maxInfoSize  = textRect.size();
    QSizeF labelSize;
    QSizeF infoSize;

    // If we have additional info text, and there's space for at least two lines of text,
    // adjust the max label size to make room for at least one line of the info text
    if (!infoText.isEmpty() && textRect.height() >= fm.lineSpacing() * 2)
    {
        infoLayout->setFont(labelLayout->font());
        infoLayout->setTextOption(labelLayout->textOption());

        maxLabelSize.rheight() -= fm.lineSpacing();
        showInformation = true;
    }

    // Lay out the label text, and adjust the max info size based on the label size
    labelSize = layoutText(*labelLayout, option, text, maxLabelSize);
    maxInfoSize.rheight() -= labelSize.height();

    // Lay out the info text
    if (showInformation) {
        infoSize = layoutText(*infoLayout, option, infoText, maxInfoSize);
    } else {
        infoSize = QSizeF(0, 0);
    }
    // Compute the bounding rect of the text
    const Qt::Alignment alignment = labelLayout->textOption().alignment();
    const QSizeF size(qMax(labelSize.width(), infoSize.width()), labelSize.height() + infoSize.height());
    *textBoundingRect = QStyle::alignedRect(iconDirection(option), alignment, size.toSize(), textRect.toRect());

    // Compute the positions where we should draw the layouts
    labelLayout->setPosition(QPointF(textRect.x(), textBoundingRect->y()));
    infoLayout->setPosition(QPointF(textRect.x(), textBoundingRect->y() + labelSize.height()));
    //kDebug() << "final position is" << labelLayout->position();
}

QBrush IconPrivate::foregroundBrush(const QStyleOptionGraphicsItem *option) const
{
    const QPalette::ColorGroup group = option->state & QStyle::State_Enabled ?
            QPalette::Normal : QPalette::Disabled;

    // Always use the highlight color for selected items
    if (option->state & QStyle::State_Selected) {
        return option->palette.brush(group, QPalette::HighlightedText);
    }
    return option->palette.brush(group, QPalette::Text);
}

QBrush IconPrivate::backgroundBrush(const QStyleOptionGraphicsItem *option) const
{
    const QPalette::ColorGroup group = option->state & QStyle::State_Enabled ?
            QPalette::Normal : QPalette::Disabled;

    QBrush background(Qt::NoBrush);

    // Always use the highlight color for selected items
    if (option->state & QStyle::State_Selected) {
        background = option->palette.brush(group, QPalette::Highlight);
    }
    return background;
}

void IconPrivate::drawTextItems(QPainter *painter, const QStyleOptionGraphicsItem *option,
                                  const QTextLayout &labelLayout, const QTextLayout &infoLayout) const
{
    Q_UNUSED(option)

    painter->save();
    painter->setPen(textColor);

    // the translation prevents odd rounding errors in labelLayout.position()
    // when applied to the canvas
    painter->translate(0.5, 0.5);

    labelLayout.draw(painter, QPointF());

    if (!infoLayout.text().isEmpty()) {
        painter->setPen(textColor);
        infoLayout.draw(painter, QPointF());
    }
    painter->restore();
}


void Icon::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(widget)

#ifdef BACKINGSTORE_BLUR_HACK
     if (d->state == IconPrivate::HoverState && scene()) {
         QList<QGraphicsView*> views = scene()->views();
         if (views.count() > 0) {
             QPixmap* pix = static_cast<QPixmap*>(views[0]->windowSurface()->paintDevice());
             QImage image(boundingRect().size().toSize(), QImage::Format_ARGB32_Premultiplied);
             {
                 QPainter p(&image);
                 p.drawPixmap(image.rect(), *pix, sceneBoundingRect());
             }
             expblur<16,7>(image, 8);
             painter->drawImage(0, 0, image);
         }
     }
#endif

    //Lay out the main icon and action icons
    layoutIcons(option);

    // Compute the metrics, and lay out the text items
    // ========================================================================
    IconPrivate::IconState state = IconPrivate::NoState;
    if (d->states & IconPrivate::ManualPressedState) {
        state = IconPrivate::PressedState;
    } else if (d->states & IconPrivate::PressedState) {
        if (d->states & IconPrivate::HoverState) {
            state = IconPrivate::PressedState;
        }
    } else if (d->states & IconPrivate::HoverState) {
        state = IconPrivate::HoverState;
    }

    QPixmap icon = d->decoration(option, state != IconPrivate::NoState);
    const QPointF iconPos = d->iconPosition(option, icon);

    d->drawBackground(painter, state);

    // draw icon
    if (!icon.isNull()) {
        painter->drawPixmap(iconPos, icon);
    }

    // Draw corner actions
    foreach (const IconAction *action, d->cornerActions) {
        if (action->animationId()) {
            action->paint(painter);
        }
    }

    // Draw text last because its overlayed
    QTextLayout labelLayout, infoLayout;
    QRectF textBoundingRect;
    d->layoutTextItems(option, icon, &labelLayout, &infoLayout, &textBoundingRect);

    QImage shadow(textBoundingRect.size().toSize()+QSize(4,4), QImage::Format_ARGB32_Premultiplied);
    shadow.fill(Qt::transparent);
    {
        QPainter buffPainter(&shadow);
        buffPainter.translate(-textBoundingRect.x(), -textBoundingRect.y());
        d->drawTextItems(&buffPainter, option, labelLayout, infoLayout);
    }

    QPoint shadowOffset = QPoint(1,2);
    if (d->shadowColor.value() > 128) {
        shadowOffset = QPoint(0,1);
    }

    PaintUtils::shadowBlur(shadow, 2, d->shadowColor);
    painter->drawImage(textBoundingRect.topLeft()+shadowOffset, shadow);
    d->drawTextItems(painter, option, labelLayout, infoLayout);
}

void Icon::drawActionButtonBase(QPainter* painter, const QSize &size, int element)
{
    qreal radius = size.width()/2;
    QRadialGradient gradient(radius, radius, radius, radius, radius);
    int alpha;

    if (element == IconPrivate::MinibuttonPressed) {
        alpha = 255;
    } else if (element == IconPrivate::MinibuttonHover) {
        alpha = 200;
    } else {
        alpha = 160;
    }
    gradient.setColorAt(0, QColor::fromRgb(d->textColor.red(),
                                           d->textColor.green(),
                                           d->textColor.blue(), alpha));
    gradient.setColorAt(1, QColor::fromRgb(d->textColor.red(),
                                           d->textColor.green(),
                                           d->textColor.blue(), 0));

    painter->setBrush(gradient);
    painter->setPen(Qt::NoPen);
    painter->drawEllipse(QRectF(QPointF(.0, .0), size));
}


void Icon::setText(const QString& text)
{
    d->text = text;
    // cause a relayout
    d->currentSize = QSizeF(-1, -1);
    //try to relayout, needed if an icon was never shown before
    if (!isVisible()) {
        QStyleOptionGraphicsItem styleoption;
        layoutIcons(&styleoption);
    }
    resize(sizeFromIconSize(d->iconSize.width()));
}

QString Icon::text() const
{
    return d->text;
}

void Icon::setInfoText(const QString& text)
{
    d->infoText = text;
    // cause a relayout
    d->currentSize = QSizeF(-1, -1);
    //try to relayout, needed if an icon was never shown before
    if (!isVisible()) {
        layoutIcons(new QStyleOptionGraphicsItem);
    }
    resize(sizeFromIconSize(d->iconSize.width()));
}

QString Icon::infoText() const
{
    return d->infoText;
}

QIcon Icon::icon() const
{
    return d->icon;
}

void Icon::setIcon(const QString& icon)
{
    if (icon.isEmpty()) {
        setIcon(QIcon());
        return;
    }

    setIcon(KIcon(icon));
}

void Icon::setIcon(const QIcon& icon)
{
    d->icon = icon;
}

QSizeF Icon::iconSize() const
{
    return d->iconSize;
}

bool Icon::isDown()
{
    return d->states & IconPrivate::PressedState;
}

void Icon::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        QGraphicsWidget::mousePressEvent(event);
        return;
    }

    d->states |= IconPrivate::PressedState;
    d->clickStartPos = scenePos();

    bool handled = false;
    foreach (IconAction *action, d->cornerActions) {
        handled = action->event(event->type(), event->pos());
        if (handled) {
            break;
        }
    }

    if (!handled && geometry().contains(event->pos())) {
        emit pressed(true);
    }

    update();
}

void Icon::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    if (~d->states & IconPrivate::PressedState) {
        QGraphicsWidget::mouseMoveEvent(event);
        return;
    }

    if (boundingRect().contains(event->pos())) {
        if (~d->states & IconPrivate::HoverState) {
            d->states |= IconPrivate::HoverState;
            update();
        }
    } else {
        if (d->states & IconPrivate::HoverState) {
            d->states &= ~IconPrivate::HoverState;
            update();
        }
    }
}

void Icon::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    if (~d->states & IconPrivate::PressedState) {
        QGraphicsWidget::mouseMoveEvent(event);
        return;
    }

    d->states &= ~IconPrivate::PressedState;

    //don't pass click when the mouse was moved
    bool handled = d->clickStartPos != scenePos();
    if (!handled) {
        foreach (IconAction *action, d->cornerActions) {
            if (action->event(event->type(), event->pos())) {
                handled = true;
                break;
            }
        }
    }

    if (!handled) {
        if (boundingRect().contains(event->pos())) {
            emit clicked();
            if (KGlobalSettings::singleClick()) {
               emit activated();
            }
        }
        emit pressed(false);
    }

    update();
}

void Icon::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event)

    emit doubleClicked();
    if (!KGlobalSettings::singleClick()) {
        emit activated();
    }
}

void Icon::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    foreach (IconAction *action, d->cornerActions) {
        action->show();
        action->event(event->type(), event->pos());
    }
    hoverEffect(true);
    update();

    QGraphicsWidget::hoverEnterEvent(event);
}

void Icon::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    foreach (IconAction *action, d->cornerActions) {
        action->hide();
        action->event(event->type(), event->pos());
    }
    // d->states &= ~IconPrivate::HoverState; // Will be set once progress is zero again ... 
    hoverEffect(false);
    update();

    QGraphicsWidget::hoverLeaveEvent(event);
}

void Icon::setPressed(bool pressed)
{
    if (pressed) {
        d->states |= IconPrivate::ManualPressedState;
        d->states |= IconPrivate::PressedState;
    } else {
        d->states &= ~IconPrivate::ManualPressedState;
        d->states &= ~IconPrivate::PressedState;
    }
    update();
}

void Icon::setUnpressed()
{
    setPressed(false);
}

void IconPrivate::syncToAction()
{
    if (!action) {
        return;
    }
    //we don't get told *what* changed, just that something changed
    //so we update everything we care about
    q->setIcon(action->icon());
    q->setText(action->iconText());
    q->setEnabled(action->isEnabled());
    //TODO use action's tooltip too

    emit q->changed();
}

void Icon::setOrientation(Qt::Orientation orientation)
{
    d->orientation = orientation;
    resize(sizeFromIconSize(d->iconSize.width()));
}

void Icon::invertLayout(bool invert)
{
    d->invertLayout = invert;
}

bool Icon::invertedLayout() const
{
    return d->invertLayout;
}

QSizeF Icon::sizeFromIconSize(const qreal iconWidth) const
{
    if (d->text.isEmpty() && d->infoText.isEmpty()) {
        //no text, less calculations
        return d->addMargin(d->addMargin(QSizeF(iconWidth, iconWidth), IconPrivate::IconMargin),
                            IconPrivate::ItemMargin);
    }

    QFontMetricsF fm = Plasma::Theme::defaultTheme()->fontMetrics();
    qreal width = 0;

    if (d->orientation == Qt::Vertical) {
        // make room for at most 14 characters
        width = qMax(fm.width(d->text.left(12)),
                     fm.width(d->infoText.left(12))) +
                     fm.width("xx") +
                     d->horizontalMargin[IconPrivate::TextMargin].left +
                     d->horizontalMargin[IconPrivate::TextMargin].right;

        width = qMax(width,
                     iconWidth +
                     d->horizontalMargin[IconPrivate::IconMargin].left +
                     d->horizontalMargin[IconPrivate::IconMargin].right);
    } else {
        width = iconWidth +
                d->horizontalMargin[IconPrivate::IconMargin].left +
                d->horizontalMargin[IconPrivate::IconMargin].right +
                qMax(fm.width(d->text), fm.width(d->infoText)) + fm.width("xx") +
                d->horizontalMargin[IconPrivate::TextMargin].left +
                d->horizontalMargin[IconPrivate::TextMargin].right;
    }

    qreal height;
    qreal textHeight;

    QStyleOptionGraphicsItem option;
    option.state = QStyle::State_None;
    option.rect = boundingRect().toRect();
    textHeight = d->displaySizeHint(&option, width).height();

    if (d->orientation == Qt::Vertical) {
        height = iconWidth + textHeight +
                 d->verticalMargin[IconPrivate::TextMargin].top +
                 d->verticalMargin[IconPrivate::TextMargin].bottom +
                 d->verticalMargin[IconPrivate::IconMargin].top +
                 d->verticalMargin[IconPrivate::IconMargin].bottom;
    } else {
        //Horizontal
        height = qMax(iconWidth +
                      d->verticalMargin[IconPrivate::IconMargin].top +
                      d->verticalMargin[IconPrivate::IconMargin].bottom,
                      textHeight +
                      d->verticalMargin[IconPrivate::TextMargin].top +
                      d->verticalMargin[IconPrivate::TextMargin].bottom);
    }

    return d->addMargin(QSizeF(width, height), IconPrivate::ItemMargin);
}

} // namespace Plasma

#include "icon.moc"
