/*
 *  SPDX-FileCopyrightText: 2025 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "imageset.h"

#include <QDirIterator>
#include <QTest>

class ImageSetTest : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private Q_SLOTS:
    void testBasePath();

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

void ImageSetTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    m_themeDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % '/' % "plasma");
    m_themeDir.removeRecursively();

    copyDirectory(QFINDTESTDATA("data/plasma"), m_themeDir.absolutePath());
}

void ImageSetTest::cleanupTestCase()
{
    m_themeDir.removeRecursively();
}

void ImageSetTest::testBasePath()
{
    KSvg::ImageSet set("testtheme", "plasma/desktoptheme");
    QCOMPARE(set.imageSetName(), "testtheme");
    QCOMPARE(set.basePath(), "plasma/desktoptheme/");

    set.setImageSetName("test_old_metadata_format_theme");
    QCOMPARE(set.imageSetName(), "test_old_metadata_format_theme");
    QCOMPARE(set.basePath(), "plasma/desktoptheme/");
}

QTEST_MAIN(ImageSetTest)

#include "imagesettest.moc"
