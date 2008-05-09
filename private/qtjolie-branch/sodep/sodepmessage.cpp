/**
  * This file is part of the KDE project
  * Copyright (C) 2008 Kevin Ottens <ervin@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "sodepmessage.h"

#include <QtCore/QString>

#include "sodephelpers_p.h"

class SodepMessagePrivate
{
public:
    QString resourcePath;
    QString operationName;
    SodepFault fault;
    SodepValue data;
};

SodepMessage::SodepMessage()
    : d(new SodepMessagePrivate)
{
}

SodepMessage::SodepMessage(const QString &resourcePath, const QString &operationName)
    : d(new SodepMessagePrivate)
{
    d->resourcePath = resourcePath;
    d->operationName = operationName;
}

SodepMessage::SodepMessage(const SodepMessage &other)
    : d(new SodepMessagePrivate)
{
    *d = *other.d;
}

SodepMessage::~SodepMessage()
{
    delete d;
}

SodepMessage &SodepMessage::operator=(const SodepMessage &other)
{
    *d = *other.d;

    return *this;
}

QString SodepMessage::resourcePath() const
{
    return d->resourcePath;
}

QString SodepMessage::operationName() const
{
    return d->operationName;
}

SodepFault SodepMessage::fault() const
{
    return d->fault;
}

void SodepMessage::setFault(const SodepFault &fault)
{
    d->fault = fault;
}

SodepValue SodepMessage::data() const
{
    return d->data;
}

void SodepMessage::setData(const SodepValue &data)
{
    d->data = data;
}

bool SodepMessage::isValid()
{
    return !d->resourcePath.isEmpty() && !d->operationName.isEmpty();
}

void SodepMessage::writeTo(QIODevice &io) const
{
    sodepWrite(io, d->resourcePath);
    sodepWrite(io, d->operationName);
    d->fault.writeTo(io);
    d->data.writeTo(io);
}

SodepMessage SodepMessage::readFrom(QIODevice &io)
{
    SodepMessage result;

    result.d->resourcePath = sodepReadString(io);
    result.d->operationName = sodepReadString(io);
    result.d->fault = SodepFault::readFrom(io);
    result.d->data = SodepValue::readFrom(io);

    return result;
}

