/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QDirIterator>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QQuickView>
#include <QStandardPaths>
#include <QTest>

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
    void stretchedElementKeepsItsColour();

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

void ItemTest::stretchedElementKeepsItsColour()
{
    // An element which repeats along an axis is drawn from a texture of its own size and stretched, so what
    // the item shows has to be what rendering it at the item's size would have shown. A flat element makes
    // that checkable: every pixel of it, and of the item, is one colour.
    auto set = std::make_unique<KSvg::ImageSet>();
    set->setBasePath("plasma/desktoptheme");
    set->setImageSetName("testtheme");

    const QString path = QFINDTESTDATA("data/stretchborders.svg");
    QVERIFY(!path.isEmpty());

    KSvg::Svg reference;
    reference.setImageSet(set.get());
    reference.setImagePath(path);
    reference.setContainsMultipleImages(true);
    QVERIFY(reference.isValid());
    const QImage own = reference.image(reference.elementSize(QStringLiteral("center")).toSize(), QStringLiteral("center"));
    QVERIFY(!own.isNull());
    const QColor expected = own.pixelColor(own.width() / 2, own.height() / 2);

    QQuickView view;
    view.engine()->rootContext()->setContextProperty(QStringLiteral("stretchImagePath"), path);
    view.engine()->rootContext()->setContextProperty(QStringLiteral("stretchElementId"), QStringLiteral("center"));
    view.setSource(QUrl::fromLocalFile(QFINDTESTDATA("itemstretchtest.qml")));
    QCOMPARE(view.status(), QQuickView::Ready);
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.resize(240, 40); // far wider and taller than the element, so it is stretched both ways
    view.show();
    if (!QTest::qWaitForWindowExposed(&view)) {
        QSKIP("this platform never shows the window, so there is no rendering to compare");
    }
    // The item rasterises its element while it is polished, so the first frame has to have happened.
    QTRY_VERIFY(view.rootObject() && view.rootObject()->width() > 0);
    QTest::qWait(200);

    const QImage shown = view.grabWindow();
    if (shown.isNull()) {
        QSKIP("this platform gives back no rendering to compare");
    }
    QCOMPARE(shown.size(), view.size() * view.devicePixelRatio());

    // Inside the item, away from its edges, every pixel is the element's one colour.
    for (int y = 4; y < shown.height() - 4; y += 4) {
        for (int x = 4; x < shown.width() - 4; x += 4) {
            QCOMPARE(shown.pixelColor(x, y), expected);
        }
    }
}

#include "itemtest.moc"

QTEST_MAIN(ItemTest);
