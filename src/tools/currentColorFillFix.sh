#!/bin/sh

# SPDX-License-Identifier: LGPL-2.0-or-later
# SPDX-FileCopyrightText: 2006-2023 KDE Contributors

if [ $# -ne 1 ]; 
    then echo Usage: $0 file.svgz
    exit 1
fi

if [ ! -f $1 ]; then
    echo "you must specify a valid svg"
    exit 1
fi


file=`echo $1 | cut -d'.' --complement -f2-`
mv $1 $file.svg.gz
gunzip $file.svg.gz

echo Processing $file

/usr/bin/perl -p -i -e "s/color:#[^;]*;(.*)fill:currentColor/\1fill:currentColor/g" $file.svg

gzip -n $file.svg
mv $file.svg.gz $file.svgz
