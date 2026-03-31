/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include "svg.h"

#include <QDirIterator>
#include <QSignalSpy>
#include <QTest>

using namespace Qt::Literals;

class SvgTest : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private Q_SLOTS:
    void testSize();
    void testElements();

private:
    KSvg::Svg *m_svg;
    QDir m_themeDir;
    QDir m_cacheDir;
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

void SvgTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    m_themeDir = QDir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) % '/' % "plasma");
    m_themeDir.removeRecursively();

    copyDirectory(QFINDTESTDATA("data/plasma"), m_themeDir.absolutePath());

    m_cacheDir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_cacheDir.removeRecursively();

    m_svg = new KSvg::Svg;
    m_svg->setImagePath(QFINDTESTDATA("data/background.svgz"));
    QVERIFY(m_svg->isValid());
}

void SvgTest::cleanupTestCase()
{
    m_themeDir.removeRecursively();
}

void SvgTest::testSize()
{
    QSignalSpy spy(m_svg, &KSvg::Svg::sizeChanged);

    QCOMPARE(m_svg->size(), QSizeF(148, 148));

    m_svg->resize(500, 500);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_svg->size(), QSizeF(500, 500));

    m_svg->resize();
    QCOMPARE(spy.count(), 2);
    QCOMPARE(m_svg->size(), QSizeF(148, 148));
}

void SvgTest::testElements()
{
    QVERIFY(m_svg->hasElement("center"));
    QVERIFY(m_svg->hasElement("left"));
    QVERIFY(m_svg->hasElement("right"));
    QVERIFY(m_svg->hasElement("top"));
    QVERIFY(m_svg->hasElement("bottom"));
    QVERIFY(m_svg->hasElement("topleft"));
    QVERIFY(m_svg->hasElement("topright"));
    QVERIFY(m_svg->hasElement("bottomleft"));
    QVERIFY(m_svg->hasElement("bottomright"));

    QVERIFY(m_svg->hasElement("prefix-left"));
    QVERIFY(m_svg->hasElement("prefix-right"));
    QVERIFY(m_svg->hasElement("prefix-top"));
    QVERIFY(m_svg->hasElement("prefix-bottom"));
    QVERIFY(m_svg->hasElement("prefix-topleft"));
    QVERIFY(m_svg->hasElement("prefix-topright"));
    QVERIFY(m_svg->hasElement("prefix-bottomleft"));
    QVERIFY(m_svg->hasElement("prefix-bottomright"));

    QVERIFY(m_svg->hasElement("hint-left-margin"));
    QVERIFY(m_svg->hasElement("hint-right-margin"));
    QVERIFY(m_svg->hasElement("hint-top-margin"));
    QVERIFY(m_svg->hasElement("hint-bottom-margin"));

    QVERIFY(!m_svg->hasElement("banana"));

    QCOMPARE(m_svg->elementSize("left"), QSizeF(35, 26));

    QCOMPARE(m_svg->elementRect("left").toRect(), QRect(8, 71, 35, 26));
}

QTEST_MAIN(SvgTest)

#include "svgtest.moc"
