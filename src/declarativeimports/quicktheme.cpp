/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "quicktheme.h"

namespace PlasmaSvg
{
QuickTheme::QuickTheme(QObject *parent)
    : Theme(parent)
{
    connect(this, &Theme::themeChanged, this, &QuickTheme::themeChangedProxy);
}

QuickTheme::~QuickTheme()
{
}

QColor QuickTheme::textColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor);
}

QColor QuickTheme::highlightColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HighlightColor);
}

QColor QuickTheme::highlightedTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HighlightedTextColor);
}

QColor QuickTheme::positiveTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::PositiveTextColor);
}

QColor QuickTheme::neutralTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::NeutralTextColor);
}

QColor QuickTheme::negativeTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::NegativeTextColor);
}

QColor QuickTheme::disabledTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::DisabledTextColor);
}

QColor QuickTheme::backgroundColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::BackgroundColor);
}

QColor QuickTheme::buttonTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonBackgroundColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::BackgroundColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonPositiveTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonNeutralTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonNegativeTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::linkColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::LinkColor);
}

QColor QuickTheme::visitedLinkColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::VisitedLinkColor);
}

QColor QuickTheme::buttonHoverColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HoverColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonFocusColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::FocusColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonHighlightedTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HighlightedTextColor, PlasmaSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::viewTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewBackgroundColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::BackgroundColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewHoverColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HoverColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewFocusColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::FocusColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewHighlightedTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HighlightedTextColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewPositiveTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewNeutralTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewNegativeTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::complementaryTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryBackgroundColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::BackgroundColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryHoverColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HoverColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryFocusColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::FocusColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryHighlightedTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HighlightedTextColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryPositiveTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryNeutralTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryNegativeTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::headerTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerBackgroundColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::BackgroundColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerHoverColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HoverColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerFocusColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::FocusColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerHighlightedTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::HighlightedTextColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerPositiveTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerNeutralTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerNegativeTextColor() const
{
    return PlasmaSvg::Theme::color(PlasmaSvg::Theme::TextColor, PlasmaSvg::Theme::HeaderColorGroup);
}
}

#include "moc_quicktheme.cpp"
