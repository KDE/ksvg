/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QDirIterator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QStandardPaths>
#include <QTest>

#include <QColor>

#include <KSvg/ImageSet>
#include <KSvg/Svg>

class ItemTest : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private Q_SLOTS:
    void testItem();
    void testElementDrawnAsAColor();
    void testElementDrawnFromOnePixel();

private:
    QDir m_themeDir;
};

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

void ItemTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    m_themeDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % '/' % "plasma");
    m_themeDir.removeRecursively();

    copyDirectory(QFINDTESTDATA("data/plasma"), m_themeDir.absolutePath());

    m_themeDir.mkpath("desktoptheme/testtheme/widgets");
    QVERIFY(QFile::copy(QFINDTESTDATA("data/background.svgz"), m_themeDir.absolutePath() + "/desktoptheme/testtheme/widgets/background.svgz"));
    QVERIFY(QFile::copy(QFINDTESTDATA("data/elementhint.svg"), m_themeDir.absolutePath() + "/desktoptheme/testtheme/widgets/line.svg"));
}

void ItemTest::cleanupTestCase()
{
    m_themeDir.removeRecursively();
}

void ItemTest::testItem()
{
    auto set = std::make_unique<KSvg::ImageSet>();
    set->setBasePath("plasma/desktoptheme");
    set->setImageSetName("testtheme");

    QQmlEngine engine;
    QQmlComponent comp(&engine, QFINDTESTDATA("itemtest.qml"));
    auto item = comp.create();
    QVERIFY(item);

    auto svg = item->property("svg").value<KSvg::Svg *>();
    QVERIFY(svg);

    QVERIFY(svg->isValid());
    QCOMPARE(svg->imageSet()->imageSetName(), "testtheme");
    QCOMPARE(svg->imagePath(), "widgets/background");

    QCOMPARE(item->property("naturalSize"), QSizeF(148, 148));
    QCOMPARE(item->property("elementRect"), QRectF(0, 0, 148, 148));

    delete item;
}

#include "itemtest.moc"

// An element which is one flat colour is drawn as that colour rather than as a picture of it, so the colour
// an item would fill with has to be the colour the picture would have shown, at any size.
void ItemTest::testElementDrawnAsAColor()
{
    auto set = std::make_unique<KSvg::ImageSet>();
    set->setBasePath("plasma/desktoptheme");
    set->setImageSetName("testtheme");

    KSvg::Svg svg;
    svg.setImageSet(set.get());
    svg.setImagePath("widgets/line");
    svg.setContainsMultipleImages(true);
    QVERIFY(svg.isValid());

    // The theme says so for one element and not for the other.
    QVERIFY(svg.hasElement("vertical-line-hint-solid-color"));
    QVERIFY(!svg.hasElement("shaded-line-hint-solid-color"));

    // The colour is read from a three pixel rendering, and it is the colour the element draws whatever size
    // it is asked for.
    const QColor sampled = svg.image(QSize(3, 3), "vertical-line").pixelColor(1, 1);
    QCOMPARE(sampled, QColor(0x20, 0x4a, 0x87));
    for (const QSize &size : {QSize(1, 1), QSize(8, 280), QSize(3, 1024)}) {
        const QImage drawn = svg.image(size, "vertical-line");
        QCOMPARE(drawn.pixelColor(size.width() / 2, size.height() / 2), sampled);
    }

    // And the element which shades along its length is not one colour, so nothing about it is flattened.
    const QImage shaded = svg.image(QSize(1, 64), "shaded-line");
    QVERIFY(shaded.pixelColor(0, 4) != shaded.pixelColor(0, 60));
}

// An element whose picture repeats along an axis is drawn from one pixel there and stretched, so one pixel
// of it has to be the same picture as the width the item asks for.
void ItemTest::testElementDrawnFromOnePixel()
{
    auto set = std::make_unique<KSvg::ImageSet>();
    set->setBasePath("plasma/desktoptheme");
    set->setImageSetName("testtheme");

    KSvg::Svg svg;
    svg.setImageSet(set.get());
    svg.setImagePath("widgets/line");
    svg.setContainsMultipleImages(true);
    QVERIFY(svg.isValid());

    // The theme says it of the element which shades down its height and is alike across its width.
    QVERIFY(svg.hasElement("shaded-line-hint-stretch-horizontally"));
    QVERIFY(!svg.hasElement("shaded-line-hint-stretch-vertically"));

    // One column of it, grown across, is what the item would have rendered at its own width.
    const int height = 64;
    const QImage narrow = svg.image(QSize(1, height), "shaded-line");
    const QImage wide = svg.image(QSize(40, height), "shaded-line");
    QVERIFY(!narrow.isNull() && !wide.isNull());
    for (int y = 0; y < height; ++y) {
        QCOMPARE(narrow.pixelColor(0, y), wide.pixelColor(20, y));
    }
}

QTEST_MAIN(ItemTest);
