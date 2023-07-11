#!/bin/sh

# SPDX-License-Identifier: LGPL-2.0-or-later
# SPDX-FileCopyrightText: 2006-2023 KDE Contributors

#If this test fails it means you are probably using i18n() in your QML code
#This should be replaced by i18nd in order to for i18n to load the correct catalog

#First arg should be the directory to check

! find "$1" -name '*.qml' -print0 | xargs -0 grep 'i18n[^d]*('

