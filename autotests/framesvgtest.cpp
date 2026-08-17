/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "framesvgtest.h"
#include <QDirIterator>
#include <QStandardPaths>

void copyDirectory(const QString &srcDir, const QString &dstDir)
{
    QDir targetDir(dstDir);
    QDirIterator it(srcDir, QDir::Filters(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Name), QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString path = it.filePath();
        QString relDestPath = path.last(it.filePath().length() - srcDir.length() - 1);
        if (it.fileInfo().isDir()) {
            QVERIFY(targetDir.mkpath(relDestPath));
        } else {
            QVERIFY(QFile::copy(path, dstDir % '/' % relDestPath));
        }
    }
}

void FrameSvgTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    m_themeDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % '/' % "plasma");
    m_themeDir.removeRecursively();

    copyDirectory(QFINDTESTDATA("data/plasma"), m_themeDir.absolutePath());

    m_cacheDir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_cacheDir.removeRecursively();

    m_frameSvg = new KSvg::FrameSvg;
    m_frameSvg->setImagePath(QFINDTESTDATA("data/background.svgz"));
    QVERIFY(m_frameSvg->isValid());
}

void FrameSvgTest::cleanupTestCase()
{
    delete m_frameSvg;

    m_themeDir.removeRecursively();
    m_cacheDir.removeRecursively();
}

void FrameSvgTest::margins()
{
    QCOMPARE(m_frameSvg->marginSize(KSvg::FrameSvg::LeftMargin), (qreal)26);
    QCOMPARE(m_frameSvg->marginSize(KSvg::FrameSvg::TopMargin), (qreal)26);
    QCOMPARE(m_frameSvg->marginSize(KSvg::FrameSvg::RightMargin), (qreal)26);
    QCOMPARE(m_frameSvg->marginSize(KSvg::FrameSvg::BottomMargin), (qreal)26);
}

void FrameSvgTest::contentsRect()
{
    m_frameSvg->resizeFrame(QSize(100, 100));
    QCOMPARE(m_frameSvg->contentsRect(), QRectF(26, 26, 48, 48));
}

void FrameSvgTest::repaintBlocked()
{
    // check the properties to be correct even if set during a repaint blocked transaction
    m_frameSvg->setRepaintBlocked(true);
    QVERIFY(m_frameSvg->isRepaintBlocked());

    m_frameSvg->setElementPrefix("prefix");
    m_frameSvg->setEnabledBorders(KSvg::FrameSvg::TopBorder | KSvg::FrameSvg::LeftBorder);
    m_frameSvg->resizeFrame(QSizeF(100, 100));

    m_frameSvg->setRepaintBlocked(false);

    QCOMPARE(m_frameSvg->prefix(), QString("prefix"));
    QCOMPARE(m_frameSvg->enabledBorders(), KSvg::FrameSvg::TopBorder | KSvg::FrameSvg::LeftBorder);
    QCOMPARE(m_frameSvg->frameSize(), QSizeF(100, 100));
}

void FrameSvgTest::setImageSet()
{
    // Should not crash

    KSvg::FrameSvg *frameSvg = new KSvg::FrameSvg;
    frameSvg->setImagePath("widgets/background");
    frameSvg->setImageSet(new KSvg::ImageSet("breeze-light", {}, this));
    frameSvg->framePixmap();
    frameSvg->setImageSet(new KSvg::ImageSet("breeze-dark", {}, this));
    frameSvg->framePixmap();
    delete frameSvg;

    frameSvg = new KSvg::FrameSvg;
    frameSvg->setImagePath("widgets/background");
    frameSvg->setImageSet(new KSvg::ImageSet("breeze-light", {}, this));
    frameSvg->framePixmap();
    frameSvg->setImageSet(new KSvg::ImageSet("breeze-dark", {}, this));
    frameSvg->framePixmap();

    frameSvg->setImageSet(new KSvg::ImageSet("testtheme", "plasma/desktoptheme", this));
    QCOMPARE(frameSvg->color(KSvg::Svg::Text), QColor(255, 54, 59));

    delete frameSvg;
}

void FrameSvgTest::resizeMask()
{
    m_frameSvg->resizeFrame(QSize(100, 100));
    QCOMPARE(m_frameSvg->alphaMask().size(), QSize(100, 100));
    m_frameSvg->resizeFrame(QSize(50, 50));
    QCOMPARE(m_frameSvg->alphaMask().size(), QSize(50, 50));
    m_frameSvg->resizeFrame(QSize(100, 100));
    QCOMPARE(m_frameSvg->alphaMask().size(), QSize(100, 100));
}

void FrameSvgTest::solidCenter()
{
    // When the "center" element rasterizes to a single color, the frame interior must render as that
    // exact color. This is the case the flat-fill fast path optimizes, and it must stay pixel-correct.
    m_frameSvg->setElementPrefix(QString());
    m_frameSvg->setEnabledBorders(KSvg::FrameSvg::AllBorders);
    m_frameSvg->resizeFrame(QSize(100, 100));

    const QImage frame = m_frameSvg->framePixmap().toImage();
    QCOMPARE(frame.size(), QSize(100, 100));

    // The color the "center" element itself rasterizes to.
    const QImage center =
        m_frameSvg->image(m_frameSvg->elementSize(QStringLiteral("center")).toSize(), QStringLiteral("center")).convertToFormat(QImage::Format_ARGB32);
    QVERIFY(!center.isNull());
    const QColor expected = QColor::fromRgba(center.pixel(0, 0));
    QVERIFY(expected.alpha() > 0);

    // Every pixel of the content rect (borders are 26px), inset slightly to stay clear of the
    // antialiased border seam, must be exactly that color.
    const QRect interior = QRect(26, 26, 48, 48).adjusted(2, 2, -2, -2);
    for (int y = interior.top(); y <= interior.bottom(); ++y) {
        for (int x = interior.left(); x <= interior.right(); ++x) {
            QCOMPARE(frame.pixelColor(x, y), expected);
        }
    }
}

void FrameSvgTest::roundedCenterIsNotFlat()
{
    // A centre which is a circle 2 pixels across comes out one colour at its own size, by symmetry, so
    // a check made there would take it for a flat colour and fill the content rect with it. Stretched
    // over the frame the circle plainly is not flat, and its corners must stay clear.
    KSvg::FrameSvg frame;
    frame.setImagePath(QFINDTESTDATA("data/roundedcenter.svg"));
    QVERIFY(frame.isValid());
    frame.setEnabledBorders(KSvg::FrameSvg::AllBorders);
    frame.resizeFrame(QSize(100, 100));

    const QImage image = frame.framePixmap().toImage();
    QCOMPARE(image.size(), QSize(100, 100));

    const QRect content = frame.contentsRect().toRect();
    QVERIFY(content.width() > 20 && content.height() > 20);

    // The middle of the content rect is inside the circle, its corner is outside it.
    const int middleAlpha = image.pixelColor(content.center()).alpha();
    const int cornerAlpha = image.pixelColor(content.topLeft() + QPoint(1, 1)).alpha();
    QVERIFY(middleAlpha > 0);
    QVERIFY2(cornerAlpha < middleAlpha,
             qPrintable(QStringLiteral("the centre was painted as a flat colour: corner alpha %1, middle alpha %2").arg(cornerAlpha).arg(middleAlpha)));
}

void FrameSvgTest::stretchingABorderMatchesRenderingIt()
{
    // What the fast path for borders rests on: a border which repeats along its length is the same
    // picture whether it is rendered at the frame's size or stretched from its own, so the frame can
    // hand the small texture to the GPU and let it stretch. A border which changes along its length is
    // not, and must keep being rendered at size.
    KSvg::FrameSvg frame;
    frame.setImagePath(QFINDTESTDATA("data/stretchborders.svg"));
    QVERIFY(frame.isValid());

    // Not a whole multiple of the element, so that a nearest neighbour stretch has to land between the
    // source pixels rather than on them.
    const QSize repeating = frame.elementSize(QStringLiteral("top")).toSize();
    const QSize wide(repeating.width() * 8 + 3, repeating.height());
    const QImage stretched = frame.image(repeating, QStringLiteral("top")).scaled(wide, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QCOMPARE(stretched, frame.image(wide, QStringLiteral("top")));

    const QSize changing = frame.elementSize(QStringLiteral("bumpy-top")).toSize();
    const QSize changingWide(changing.width() * 8 + 3, changing.height());
    const QImage changingStretched = frame.image(changing, QStringLiteral("bumpy-top")).scaled(changingWide, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QVERIFY(changingStretched != frame.image(changingWide, QStringLiteral("bumpy-top")));
}

void FrameSvgTest::aCenterWhichRepeatsOneWayStretchesAlongThatAxisOnly()
{
    // What the fast path for a centre which is not one colour rests on. A centre shading down its height
    // is every column alike, so widening it is a stretch of its own narrow rendering, and the frame needs
    // no more pixels across than the element has. Along the axis it shades, it is not: rendering it short
    // and stretching that would band the gradient, so that axis has to keep the frame's size.
    KSvg::FrameSvg frame;
    frame.setImagePath(QFINDTESTDATA("data/gradientcenter.svg"));
    QVERIFY(frame.isValid());

    const QSize own = frame.elementSize(QStringLiteral("center")).toSize();
    QVERIFY(!own.isEmpty());

    // Not a whole multiple of the element, so a nearest neighbour stretch has to land between the source
    // pixels rather than on them.
    const QSize wide(own.width() * 8 + 3, own.height() * 4 + 1);
    const QImage acrossItsWidth =
        frame.image(QSize(own.width(), wide.height()), QStringLiteral("center")).scaled(wide, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QCOMPARE(acrossItsWidth, frame.image(wide, QStringLiteral("center")));

    const QImage downItsHeight =
        frame.image(QSize(wide.width(), own.height()), QStringLiteral("center")).scaled(wide, Qt::IgnoreAspectRatio, Qt::FastTransformation);
    QVERIFY(downItsHeight != frame.image(wide, QStringLiteral("center")));
}

void FrameSvgTest::aPartWhichDrawsNothingLeavesItsAreaAlone()
{
    // A part whose element exists but draws nothing, which is the shape of a shadow frame's centre, is
    // skipped rather than rendered and composed. What that must not do is skip anything which does draw,
    // so the borders around the empty centre have to come out as before.
    KSvg::FrameSvg frame;
    frame.setImagePath(QFINDTESTDATA("data/gradientcenter.svg"));
    QVERIFY(frame.isValid());
    frame.setElementPrefix(QStringLiteral("empty"));
    frame.setEnabledBorders(KSvg::FrameSvg::AllBorders);
    frame.resizeFrame(QSize(240, 120));

    const QImage image = frame.framePixmap().toImage().convertToFormat(QImage::Format_ARGB32);
    QCOMPARE(image.size(), QSize(240, 120));

    const QRect content = frame.contentsRect().toRect().adjusted(2, 2, -2, -2);
    QVERIFY(content.width() > 20 && content.height() > 20);
    for (int y = content.top(); y <= content.bottom(); y += 4) {
        for (int x = content.left(); x <= content.right(); x += 4) {
            QCOMPARE(image.pixelColor(x, y).alpha(), 0);
        }
    }

    // The borders are opaque in the fixture, and they are what must survive the skipping.
    QCOMPARE(image.pixelColor(image.width() / 2, 1).alpha(), 255);
    QCOMPARE(image.pixelColor(1, image.height() / 2).alpha(), 255);
}

void FrameSvgTest::loadQrc()
{
    KSvg::FrameSvg *frameSvg = new KSvg::FrameSvg;
    frameSvg->setImageSet(new KSvg::ImageSet("testtheme", "plasma/desktoptheme", this));
    frameSvg->setImagePath(QStringLiteral("qrc:/data/background.svgz"));
    QVERIFY(frameSvg->isValid());
    // An external image is colored as well
    QCOMPARE(frameSvg->color(KSvg::Svg::Text), QColor(255, 54, 59));
    delete frameSvg;
}

QTEST_MAIN(FrameSvgTest)

#include "moc_framesvgtest.cpp"
