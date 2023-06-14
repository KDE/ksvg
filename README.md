# KSvg
A library for rendering SVG-based themes with stylesheet recoloring and on disk cache

## Introduction
KSvg provides both C++ classes and QtQuick components to render svgs based on image packs.
Compared to plain QSvg it caches the rendered images on disk with KImageCache and can recolor
properly crafted svg shapes with stylesheet.

By default will recolor with the application palette making it possible to make UI elements in SVG

## C++
In C++ there are 3 main classes, usable also in QWidget aplications with a QPainter compatible API:

* ``ImageSet``: Used to tell the other classes where to find the SVG files: by default, SVG "themes"
                will be searched in the application data dir (share/application_name/theme_name)
* ``Svg``: Class to used to render Svg files: it loads a file with ``setImagePath`` using relative paths
            to the theme specified in ``ImageSet``
* ``FrameSvg``: A subclass of Svg usedto render 9 patch images, such as Buttons, where you want to stretch
                only the central area but not the edges

## QML
The QML bindings are imported under the ``org.kde.ksvg 1.0 as KSvg`` name.

ImageSet is exported directly to QML which makes possible to sett he theme also from QML side.

Svg and FrameSvg have correspondences called ``SvgItem`` and ``FrameSvgItem`` which inherit from QQuickItem
and will paint their associated ``Svg`` or ``FrameSvg`` stretched to their full geometry.

Code example:

```
FrameSvgItem {
    imagePath: "widgets/button" // This resolves to a file like /usr/share/myapp/mytheme/widgets/button.svgz
    prefix: "pressed"
}
```

## More documentation
Those assume the theme filesystem hyerarchy used by the Plasma shell, but the general concepts apply everywhere

* https://develop.kde.org/docs/plasma/theme/theme-elements/
* https://develop.kde.org/docs/plasma/theme/quickstart/
