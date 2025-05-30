/*
    SPDX-FileCopyrightText: 2014 Aleix Pol Gonzalez <aleixpol@blue-systems.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef IMAGETEXTURESCACHE_H
#define IMAGETEXTURESCACHE_H

#include <QQuickWindow>
#include <QSharedPointer>

class QImage;
class QSGTexture;
class ImageTexturesCachePrivate;

/*
 * Helps to manage textures by creating images and reference counts them.
 *
 * Use this class as a factory for textures, when creating them from a QImage
 * instance. Keeps track of all the created textures in a map between the
 * QImage::cacheKey() and the cached texture until it gets de-referenced.
 *
 * \sa ManagedTextureNode
 */
class ImageTexturesCache
{
public:
    ImageTexturesCache();
    ~ImageTexturesCache();

    /*!
     * Returns the texture for a given window and image.
     *
     * If image id is the same as one already provided before, we will not
     * create a new texture, and will instead return a shared pointer to the existing texture.
     */
    QSharedPointer<QSGTexture> loadTexture(QQuickWindow *window, const QImage &image, QQuickWindow::CreateTextureOptions options);

    QSharedPointer<QSGTexture> loadTexture(QQuickWindow *window, const QImage &image);

private:
    QScopedPointer<ImageTexturesCachePrivate> d;
};

#endif // IMAGETEXTURESCACHE_H
