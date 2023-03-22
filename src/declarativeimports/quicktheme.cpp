/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "quicktheme.h"

namespace KSvg
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
    return KSvg::Theme::color(KSvg::Theme::TextColor);
}

QColor QuickTheme::highlightColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HighlightColor);
}

QColor QuickTheme::highlightedTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HighlightedTextColor);
}

QColor QuickTheme::positiveTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::PositiveTextColor);
}

QColor QuickTheme::neutralTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::NeutralTextColor);
}

QColor QuickTheme::negativeTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::NegativeTextColor);
}

QColor QuickTheme::disabledTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::DisabledTextColor);
}

QColor QuickTheme::backgroundColor() const
{
    return KSvg::Theme::color(KSvg::Theme::BackgroundColor);
}

QColor QuickTheme::buttonTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonBackgroundColor() const
{
    return KSvg::Theme::color(KSvg::Theme::BackgroundColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonPositiveTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonNeutralTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonNegativeTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::linkColor() const
{
    return KSvg::Theme::color(KSvg::Theme::LinkColor);
}

QColor QuickTheme::visitedLinkColor() const
{
    return KSvg::Theme::color(KSvg::Theme::VisitedLinkColor);
}

QColor QuickTheme::buttonHoverColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HoverColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonFocusColor() const
{
    return KSvg::Theme::color(KSvg::Theme::FocusColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::buttonHighlightedTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HighlightedTextColor, KSvg::Theme::ButtonColorGroup);
}

QColor QuickTheme::viewTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewBackgroundColor() const
{
    return KSvg::Theme::color(KSvg::Theme::BackgroundColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewHoverColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HoverColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewFocusColor() const
{
    return KSvg::Theme::color(KSvg::Theme::FocusColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewHighlightedTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HighlightedTextColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewPositiveTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewNeutralTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::viewNegativeTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ViewColorGroup);
}

QColor QuickTheme::complementaryTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryBackgroundColor() const
{
    return KSvg::Theme::color(KSvg::Theme::BackgroundColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryHoverColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HoverColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryFocusColor() const
{
    return KSvg::Theme::color(KSvg::Theme::FocusColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryHighlightedTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HighlightedTextColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryPositiveTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryNeutralTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::complementaryNegativeTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::ComplementaryColorGroup);
}

QColor QuickTheme::headerTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerBackgroundColor() const
{
    return KSvg::Theme::color(KSvg::Theme::BackgroundColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerHoverColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HoverColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerFocusColor() const
{
    return KSvg::Theme::color(KSvg::Theme::FocusColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerHighlightedTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::HighlightedTextColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerPositiveTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerNeutralTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::HeaderColorGroup);
}

QColor QuickTheme::headerNegativeTextColor() const
{
    return KSvg::Theme::color(KSvg::Theme::TextColor, KSvg::Theme::HeaderColorGroup);
}
}

#include "moc_quicktheme.cpp"
