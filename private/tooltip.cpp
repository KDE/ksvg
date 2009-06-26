/*
 *   Copyright 2007 by Dan Meltzer <hydrogen@notyetimplemented.com>
 *   Copyright (C) 2008 by Alexis Ménard <darktears31@gmail.com>
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

#include "tooltip_p.h"
#include "windowpreview_p.h"

#include <QAbstractTextDocumentLayout>
#include <QBitmap>
#include <QGridLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>
#include <QTextDocument>
#include <QTimeLine>
#ifdef Q_WS_X11
#include <QX11Info>
#include <netwm.h>
#endif

#include <kdebug.h>
#include <kglobal.h>
#include <kglobalsettings.h>

#include <plasma/plasma.h>
#include <plasma/theme.h>
#include <plasma/framesvg.h>

namespace Plasma {

class TipTextWidget : public QWidget
{
public:
    TipTextWidget(ToolTip *parent)
        : QWidget(parent),
          m_toolTip(parent),
          m_document(new QTextDocument(this))
    {
        //d->text->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
//        QTextOption op;
//        op.setWrapMode(QTextOption::WordWrap);
//        m_document->setDefaultTextOption(op);
    }

    void setStyleSheet(const QString &css)
    {
        m_document->setDefaultStyleSheet(css);
    }

    void setContent(const ToolTipContent &data)
    {
        QString html;
        if (!data.mainText().isEmpty()) {
            html.append("<b>" + data.mainText() + "</b>");

            if (!data.subText().isEmpty()) {
                html.append("<br>");
            }
        }
        html.append(data.subText());

        m_anchor.clear();
        m_document->clear();
        data.registerResources(m_document);
        m_document->setHtml("<p>" + html + "</p>");
        m_document->adjustSize();
        update();
    }

    QSize minimumSizeHint() const
    {
        return m_document->size().toSize();
    }

    QSize maximumSizeHint() const
    {
        return minimumSizeHint();
    }

    void paintEvent(QPaintEvent *event)
    {
        QPainter p(this);
        m_document->drawContents(&p, event->rect());
    }

    void mousePressEvent(QMouseEvent *event)
    {
        QAbstractTextDocumentLayout *layout = m_document->documentLayout();
        if (layout) {
            m_anchor = layout->anchorAt(event->pos());
        }
    }

    void mouseReleaseEvent(QMouseEvent *event)
    {
        QAbstractTextDocumentLayout *layout = m_document->documentLayout();
        if (layout) {
            QString anchor = layout->anchorAt(event->pos());
            if (anchor == m_anchor) {
                m_toolTip->linkActivated(m_anchor, event);
            }

            m_anchor.clear();
        }
    }

private:
    ToolTip *m_toolTip;
    QTextDocument *m_document;
    QString m_anchor;
};

class ToolTipPrivate
{
    public:
        ToolTipPrivate()
        : text(0),
          imageLabel(0),
          preview(0),
          source(0),
          timeline(0),
          direction(Plasma::Up),
          autohide(true)
    { }

    TipTextWidget *text;
    QLabel *imageLabel;
    WindowPreview *preview;
    FrameSvg *background;
    QPointer<QObject> source;
    QTimeLine *timeline;
    QPoint to;
    QPoint from;
    Plasma::Direction direction;
    bool autohide;
};

ToolTip::ToolTip(QWidget *parent)
    : QWidget(parent),
      d(new ToolTipPrivate())
{
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::ToolTip);
    QGridLayout *l = new QGridLayout;
    d->preview = new WindowPreview(this);
    d->text = new TipTextWidget(this);
    d->imageLabel = new QLabel(this);
    d->imageLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);

    d->background = new FrameSvg(this);
    d->background->setImagePath("widgets/tooltip");
    d->background->setEnabledBorders(FrameSvg::AllBorders);
    updateTheme();
    connect(d->background, SIGNAL(repaintNeeded()), this, SLOT(updateTheme()));
    connect(d->preview, SIGNAL(windowPreviewClicked(WId,Qt::MouseButtons,Qt::KeyboardModifiers,QPoint)),
            this, SIGNAL(activateWindowByWId(WId,Qt::MouseButtons,Qt::KeyboardModifiers,QPoint)));
    l->addWidget(d->preview, 0, 0, 1, 2);
    l->addWidget(d->imageLabel, 1, 0);
    l->addWidget(d->text, 1, 1);
    setLayout(l);
}

ToolTip::~ToolTip()
{
    delete d;
}

void ToolTip::showEvent(QShowEvent *e)
{
    checkSize();
    QWidget::showEvent(e);
    d->preview->setInfo();
}

void ToolTip::hideEvent(QHideEvent *e)
{
    QWidget::hideEvent(e);
    if (d->source) {
        QMetaObject::invokeMethod(d->source, "toolTipHidden");
    }
}

void ToolTip::mouseReleaseEvent(QMouseEvent *event)
{
    if (rect().contains(event->pos()) &&
        (!d->preview || !d->preview->geometry().contains(event->pos()))) {
        hide();
    }
}

void ToolTip::enterEvent(QEvent *)
{
    emit hovered(true);
}

void ToolTip::leaveEvent(QEvent *)
{
    emit hovered(false);
}

void ToolTip::checkSize()
{
    //FIXME: layout bugs even on qlayouts? oh, please, no.
    d->text->setMinimumSize(0, 0);
    d->text->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    d->text->setMinimumSize(d->text->minimumSizeHint());
    d->text->setMaximumSize(d->text->maximumSizeHint());

    adjustSize();
}

void ToolTip::adjustPosition(const QSize &previous, const QSize &current)
{
    if (previous != current) {
        //offsets to stop tooltips from jumping when they resize
        int deltaX = 0;
        int deltaY = 0;
        if (d->direction == Plasma::Up) {
        /*
        kDebug() << "resizing from" << current << "to" << hint
                 << "and moving from" << pos() << "to"
                 << x() << y() + (current.height() - hint.height())
                 << current.height() - hint.height();
                 */
            deltaY = previous.height() - current.height();
        } else if (d->direction == Plasma::Left) {
        /*
        kDebug() << "vertical resizing from" << current << "to" << hint
                 << "and moving from" << pos() << "to"
                 << x() + (current.width() - hint.width()) << y()
                 << current.width() - hint.width(); */
            deltaX = previous.width() - current.width();
        }

        // resize then move if we're getting smaller, vice versa when getting bigger
        // this prevents overlap with the item in the smaller case, and a repaint of
        // the tipped item when getting bigger

        move(x() + deltaX, y() + deltaY);
    }
}

void ToolTip::setContent(QObject *tipper, const ToolTipContent &data)
{
    //reset our size
    d->text->setContent(data);
    d->imageLabel->setPixmap(data.image());
    if (data.windowsToPreview().size() > 1) {
        d->preview->setWindowIds(data.windowsToPreview());
    } else {
        QList<WId>ids;
        ids.append(data.windowToPreview());
        d->preview->setWindowIds(ids);
    }

    d->autohide = data.autohide();
    d->source = tipper;

    if (isVisible()) {
        d->preview->setInfo();
        //kDebug() << "about to check size";
        checkSize();
    }
}

void ToolTip::prepareShowing()
{
    if (!d->preview->isEmpty()) {
        // show/hide the preview area
        d->preview->show();
    } else {
        d->preview->hide();
    }

    layout()->activate();
    d->preview->setInfo();
    //kDebug() << "about to check size";
    checkSize();
}

void ToolTip::moveTo(const QPoint &to)
{
    if (!isVisible() ||
        !(KGlobalSettings::graphicEffectsLevel() & KGlobalSettings::SimpleAnimationEffects)) {
        move(to);
        return;
    }

    d->from = QPoint();
    d->to = to;

    if (!d->timeline) {
        d->timeline = new QTimeLine(250, this);
        d->timeline->setFrameRange(0, 10);
        d->timeline->setCurveShape(QTimeLine::EaseInCurve);
        connect(d->timeline, SIGNAL(valueChanged(qreal)), this, SLOT(animateMove(qreal)));
    }

    d->timeline->stop();
    d->timeline->start();
}

void ToolTip::animateMove(qreal progress)
{
    if (d->from.isNull()) {
        d->from = pos();
    }

    if (qFuzzyCompare(progress, qreal(1.0))) {
        move(d->to);
        return;
    }

    move(d->from.x() + ((d->to.x() - d->from.x()) * progress),
         d->from.y() + ((d->to.y() - d->from.y()) * progress));
}

void ToolTip::resizeEvent(QResizeEvent *e)
{
    QWidget::resizeEvent(e);
    d->background->resizeFrame(size());
    setMask(d->background->mask());
    d->preview->setInfo();

    if (isVisible()) {
        adjustPosition(e->oldSize(), e->size());
    }
}

void ToolTip::paintEvent(QPaintEvent *e)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setClipRect(e->rect());
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), Qt::transparent);

    d->background->paintFrame(&painter);
}

bool ToolTip::autohide() const
{
    return d->autohide;
}

void ToolTip::setDirection(Plasma::Direction direction)
{
    d->direction = direction;
}

void ToolTip::linkActivated(const QString &anchor, QMouseEvent *event)
{
    emit linkActivated(anchor, event->buttons(), event->modifiers(), event->globalPos());
}

void ToolTip::updateTheme()
{
    const int topHeight = d->background->marginSize(Plasma::TopMargin);
    const int leftWidth = d->background->marginSize(Plasma::LeftMargin);
    const int rightWidth = d->background->marginSize(Plasma::RightMargin);
    const int bottomHeight = d->background->marginSize(Plasma::BottomMargin);
    setContentsMargins(leftWidth, topHeight, rightWidth, bottomHeight);

    // Make the tooltip use Plasma's colorscheme
    QColor textColor = Plasma::Theme::defaultTheme()->color(Plasma::Theme::TextColor);
    QPalette plasmaPalette = QPalette();
    plasmaPalette.setColor(QPalette::Window,
                           Plasma::Theme::defaultTheme()->color(Plasma::Theme::BackgroundColor));
    plasmaPalette.setColor(QPalette::WindowText, textColor);
    setAutoFillBackground(true);
    setPalette(plasmaPalette);
    d->text->setStyleSheet(QString("p { color: %1; }").arg(textColor.name()));
    update();
}

} // namespace Plasma

#include "tooltip_p.moc"
