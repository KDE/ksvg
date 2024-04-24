/*
    SPDX-FileCopyrightText: 2023 ivan tkachenko <me@ratijas.tk>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QQmlApplicationEngine>
#include <QQmlEngine>
#include <QQuickItem>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class SvgItemTest : public QObject
{
    Q_OBJECT

public Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

private Q_SLOTS:
    void tst_create();
    void tst_implicitSize();

private:
    QDir m_cacheDir;
};

void SvgItemTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    m_cacheDir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_cacheDir.removeRecursively();
}

void SvgItemTest::cleanupTestCase()
{
    m_cacheDir.removeRecursively();
}

void SvgItemTest::tst_create()
{
    QQmlApplicationEngine app;
    app.loadData(QByteArrayLiteral(R"(
        import org.kde.ksvg as KSvg

        KSvg.SvgItem {
            imagePath: ""
            elementId: ""
        }
    )"),
                 QUrl(__FILE__));
    QVERIFY(!app.hasError());
    QCOMPARE(app.rootObjects().count(), 1);
}

// Note: Keep this test conceptually in sync with FrameSvgItemTest::tst_implicitSize
void SvgItemTest::tst_implicitSize()
{
    // Create an SvgItem
    // assign one image
    // check implicit size
    // assign another elementId to it
    // check implicit size again
    // assign another image to it
    // check implicit size again
    // override width
    // check implicit width
    // override height
    // check implicit height
    // assign first image
    // check implicit size (should stay the same)

    QString pathToBackground = QStringLiteral("file://") + QFINDTESTDATA("data/background.svg");
    QString pathToSlider = QStringLiteral("file://") + QFINDTESTDATA("data/slider.svg");

    QQmlApplicationEngine app;
    app.setInitialProperties({{QStringLiteral("imagePath"), pathToBackground}});
    app.loadData(QByteArrayLiteral(R"(
        import org.kde.ksvg as KSvg

        KSvg.SvgItem {
            elementId: "hint-top-margin"
        }
    )"),
                 QUrl(__FILE__));
    QVERIFY(!app.hasError());
    QCOMPARE(app.rootObjects().count(), 1);

    QQuickItem *frameSvgItem = qobject_cast<QQuickItem *>(app.rootObjects().first());
    QVERIFY(frameSvgItem);

    QSignalSpy implicitWidthSpy(frameSvgItem, SIGNAL(implicitWidthChanged()));
    QSignalSpy implicitHeightSpy(frameSvgItem, SIGNAL(implicitHeightChanged()));
    QVERIFY(implicitWidthSpy.isValid());
    QVERIFY(implicitHeightSpy.isValid());

    QCOMPARE(frameSvgItem->property("elementId"), QStringLiteral("hint-top-margin"));

    QCOMPARE(frameSvgItem->implicitWidth(), 4.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 26.0);

    frameSvgItem->setProperty("elementId", QString());

    QCOMPARE(implicitWidthSpy.count(), 1);
    QCOMPARE(implicitHeightSpy.count(), 1);

    QCOMPARE(frameSvgItem->implicitWidth(), 148.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 148.0);

    frameSvgItem->setProperty("imagePath", pathToSlider);
    frameSvgItem->setProperty("elementId", QStringLiteral("groove-top"));

    QCOMPARE(implicitWidthSpy.count(), 3);
    QCOMPARE(implicitHeightSpy.count(), 3);

    QCOMPARE(frameSvgItem->implicitWidth(), 3.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 3.0);

    frameSvgItem->setImplicitWidth(100.0);
    QCOMPARE(implicitWidthSpy.count(), 4);

    frameSvgItem->setProperty("elementId", QStringLiteral("horizontal-slider-shadow"));

    QCOMPARE(implicitWidthSpy.count(), 4);
    QCOMPARE(implicitHeightSpy.count(), 4);

    QCOMPARE(frameSvgItem->implicitWidth(), 100.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 26.0);

    frameSvgItem->setImplicitHeight(200.0);

    frameSvgItem->setProperty("imagePath", pathToBackground);
    frameSvgItem->setProperty("elementId", QString());

    QCOMPARE(frameSvgItem->implicitWidth(), 100.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 200.0);
}

QTEST_MAIN(SvgItemTest)

#include "svgitemtest.moc"
