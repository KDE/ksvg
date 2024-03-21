/*
    SPDX-FileCopyrightText: 2014 Marco Martin <mart@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "framesvgtest.h"
#include <QStandardPaths>

void FrameSvgTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    m_cacheDir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_cacheDir.removeRecursively();

    m_frameSvg = new KSvg::FrameSvg;
    m_frameSvg->setImagePath(QFINDTESTDATA("data/background.svgz"));
    QVERIFY(m_frameSvg->isValid());
}

void FrameSvgTest::cleanupTestCase()
{
    delete m_frameSvg;

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

void FrameSvgTest::loadQrc()
{
    KSvg::FrameSvg *frameSvg = new KSvg::FrameSvg;
    frameSvg->setImagePath(QStringLiteral("qrc:/data/background.svgz"));
    QVERIFY(frameSvg->isValid());
    delete frameSvg;
}

QTEST_MAIN(FrameSvgTest)

#include "moc_framesvgtest.cpp"
