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

void FrameSvgTest::aCenterDeclaredSolidIsFilledWithItsColour()
{
    // hint-solid-color says the centre is one colour, so the frame fills its interior with that colour
    // instead of rendering the element at the frame's size. What must hold is that the colour is the
    // element's own, read through whatever a colour scheme does to it, so the result is what rendering
    // the element gives.
    KSvg::FrameSvg frame;
    frame.setImagePath(QFINDTESTDATA("data/solidhint.svg"));
    QVERIFY(frame.isValid());
    QVERIFY(frame.hasElement(QStringLiteral("hint-solid-color")));
    frame.setEnabledBorders(KSvg::FrameSvg::AllBorders);
    frame.resizeFrame(QSize(240, 120));

    const QImage expected = frame.image(frame.elementSize(QStringLiteral("center")).toSize(), QStringLiteral("center"));
    QVERIFY(!expected.isNull());
    const QColor colour = expected.pixelColor(expected.width() / 2, expected.height() / 2);
    QVERIFY(colour.alpha() > 0);

    const QImage painted = frame.framePixmap().toImage();
    const QRect content = frame.contentsRect().toRect().adjusted(2, 2, -2, -2);
    QVERIFY(content.width() > 20 && content.height() > 20);
    for (int y = content.top(); y <= content.bottom(); y += 4) {
        for (int x = content.left(); x <= content.right(); x += 4) {
            QCOMPARE(painted.pixelColor(x, y), colour);
        }
    }
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
