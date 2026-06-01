/*
 *  SPDX-FileCopyrightText: 2026 Nicolas Fella <nicolas.fella@gmx.de>
 *
 *  SPDX-License-Identifier: LGPL-2.0-or-later
 */

#include <QDirIterator>
#include <QSignalSpy>
#include <QTest>

#include <KColorScheme>
#include <KConfigGroup>

// Cursed way to access SvgPrivate
#define private public
#include "../src/ksvg/private/svg_p.h"
#include "svg.h"

using namespace Qt::Literals;

class SvgTest : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void cleanup();

private Q_SLOTS:
    void testSize();
    void testElements();
    void testColors();
    void testStylesheetOverrideColorChange();

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

void SvgTest::cleanup()
{
    auto config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KConfigGroup cg(config, "Colors:Window");
    cg.deleteEntry("BackgroundNormal");
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

void SvgTest::testColors()
{
    KColorScheme windowColors(QPalette::Normal, KColorScheme::Window);
    KColorScheme selectionColors(QPalette::Active, KColorScheme::Selection);
    KColorScheme viewColors(QPalette::Active, KColorScheme::View);

    QCOMPARE(m_svg->color(KSvg::Svg::Text), windowColors.foreground(KColorScheme::NormalText));

    m_svg->setStatus(KSvg::Svg::Selected);
    QCOMPARE(m_svg->color(KSvg::Svg::Text), selectionColors.foreground(KColorScheme::NormalText));

    m_svg->setStatus(KSvg::Svg::Inactive);
    QCOMPARE(m_svg->color(KSvg::Svg::Text), selectionColors.foreground(KColorScheme::InactiveText));

    m_svg->setStatus(KSvg::Svg::Normal);
    m_svg->setColorSet(KSvg::Svg::View);
    QCOMPARE(m_svg->color(KSvg::Svg::Text), viewColors.foreground(KColorScheme::NormalText));

    m_svg->setStatus(KSvg::Svg::Selected);
    QCOMPARE(m_svg->color(KSvg::Svg::Text), selectionColors.foreground(KColorScheme::NormalText));

    m_svg->setStatus(KSvg::Svg::Normal);

    m_svg->setColor(KSvg::Svg::Text, QColor("#123456"));
    QCOMPARE(m_svg->color(KSvg::Svg::Text), QColor("#123456"));

    m_svg->clearColorOverrides();

    QCOMPARE(m_svg->color(KSvg::Svg::Text), viewColors.foreground(KColorScheme::NormalText));
}

void SvgTest::testStylesheetOverrideColorChange()
{
    auto config = KSharedConfig::openConfig(QStringLiteral("kdeglobals"));
    KColorScheme windowColors(QPalette::Normal, KColorScheme::Window, config);
    auto set = new KSvg::ImageSet("testthemesystemcolors", "plasma/desktoptheme");
    m_svg->setImageSet(set);
    // Set a color override in the svg
    m_svg->setColor(KSvg::Svg::Text, Qt::blue);
    // set->setImageSetName(QStringLiteral("testthemesystemcolors"));

    QSignalSpy spy(set, &KSvg::ImageSet::imageSetChanged);

    QCOMPARE(m_svg->color(KSvg::Svg::Text), Qt::blue);

    // Generate pixmap and stylesheet
    m_svg->pixmap();
    // Since we set a color, stylesheetOverride must be present
    QVERIFY(!m_svg->d->stylesheetOverride.isEmpty());

    // Simulate an application color scheme change, setting background to red
    QPalette pal = qApp->palette();
    KConfigGroup cg(config, "Colors:Window");
    cg.writeEntry("BackgroundNormal", QStringLiteral("255,0,0"));
    KColorScheme::adjustForeground(pal, KColorScheme::NormalText, QPalette::WindowText, KColorScheme::Window, config);
    config->sync();
    qApp->setPalette(pal);
    // Wait the app color change is completely done, then regenerate pixmaps and stylesheets
    QVERIFY(spy.wait());
    m_svg->pixmap();
    // The background in stylesheetoverride is now red
    QVERIFY(m_svg->d->stylesheetOverride.contains(QStringLiteral(".ColorScheme-Background{color:#ff0000;}")));
}

QTEST_MAIN(SvgTest)

#include "svgtest.moc"
