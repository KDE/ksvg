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

#include "itemtest.moc"

QTEST_MAIN(ItemTest);
