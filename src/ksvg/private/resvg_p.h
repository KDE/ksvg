// SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
// SPDX-FileCopyrightText: 2023 Harald Sitter <sitter@kde.org>

#pragma once

#include <QImage>
#include <QLocale>
#include <QPixmap>
#include <QRectF>
#include <QSize>

#include <resvg.h>

namespace std
{
template<>
struct default_delete<resvg_options> {
    void operator()(resvg_options *ptr) const
    {
        resvg_options_destroy(ptr);
    }
};

template<>
struct default_delete<resvg_render_tree> {
    void operator()(resvg_render_tree *ptr) const
    {
        resvg_tree_destroy(ptr);
    }
};
} // namespace std

namespace KSvg
{

struct ResvgOptions {
    std::unique_ptr<resvg_options> options = std::unique_ptr<resvg_options>(resvg_options_create());
};

class ResvgRenderer
{
public:
    bool load(const QByteArray &data)
    {
        resvg_render_tree *tree = nullptr;
        const auto err = resvg_parse_tree_from_data(data.constData(), data.size(), ResvgOptions().options.get(), &tree);
        if (err != RESVG_OK) {
            return false;
        }
        m_tree.reset(tree);

        const auto r = resvg_get_image_viewbox(m_tree.get());
        m_viewBox = QRectF(r.x, r.y, r.width, r.height);

        const auto s = resvg_get_image_size(m_tree.get());
        m_size = QSizeF(s.width, s.height);

        return true;
    }

    bool isValid() const
    {
        return m_tree != nullptr;
    }

    bool isEmpty() const
    {
        if (m_tree) {
            return resvg_is_image_empty(m_tree.get());
        }
        return true;
    }

    bool elementExists(const QString &id) const
    {
        if (!m_tree) {
            return false;
        }
        return resvg_node_exists(m_tree.get(), qUtf8Printable(id));
    }

    QSize defaultSize() const
    {
        return m_size.toSize();
    }

    QRectF boundsOnElement(const QString &id) const
    {
        if (!m_tree) {
            return {};
        }

        resvg_rect bbox;
        if (resvg_get_node_bbox(m_tree.get(), qUtf8Printable(id), &bbox)) {
            return {bbox.x, bbox.y, bbox.width, bbox.height};
        }

        return {};
    }

    QTransform transformForElement(const QString &id) const
    {
        if (!m_tree) {
            return {};
        }

        resvg_transform transform;
        if (resvg_get_node_transform(m_tree.get(), qUtf8Printable(id), &transform)) {
            return {transform.a, transform.b, transform.c, transform.d, transform.e, transform.f};
        }

        return {};
    }

    QPixmap render(const QRectF &finalRect) const
    {
        resvg_transform transform = resvg_transform_identity();
        const auto svgSize = defaultSize();

        QImage qimg(svgSize.width(), svgSize.height(), QImage::Format_ARGB32_Premultiplied);
        qimg.fill(Qt::transparent);
        resvg_render(m_tree.get(), transform, qimg.width(), qimg.height(), reinterpret_cast<char *>(qimg.bits()));

        // resvg renders onto the RGBA canvas, while QImage is ARGB.
        // std::move is required to call inplace version of rgbSwapped().
        return QPixmap::fromImage(std::move(qimg).rgbSwapped());
    }

    QPixmap render(const QString &elementId, const QRectF &finalRect) const
    {
        resvg_transform transform = resvg_transform_identity();
        const auto svgSize = defaultSize();

        QImage qimg(svgSize.width(), svgSize.height(), QImage::Format_ARGB32_Premultiplied);
        resvg_render_node(m_tree.get(), qUtf8Printable(elementId), transform, qimg.width(), qimg.height(), reinterpret_cast<char *>(qimg.bits()));

        // resvg renders onto the RGBA canvas, while QImage is ARGB.
        // std::move is required to call inplace version of rgbSwapped().
        return QPixmap::fromImage(std::move(qimg).rgbSwapped());
    }

    std::unique_ptr<resvg_render_tree> m_tree;
    QRectF m_viewBox;
    QSizeF m_size;
};

} // namespace KSvg
