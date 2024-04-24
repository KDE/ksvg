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

class FrameSvgItemTest : public QObject
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

void FrameSvgItemTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    m_cacheDir = QDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
    m_cacheDir.removeRecursively();
}

void FrameSvgItemTest::cleanupTestCase()
{
    m_cacheDir.removeRecursively();
}

void FrameSvgItemTest::tst_create()
{
    QQmlApplicationEngine app;
    app.loadData(QByteArrayLiteral(R"(
        import org.kde.ksvg as KSvg

        KSvg.FrameSvgItem {
            imagePath: ""
            prefix: ""
            enabledBorders: KSvg.FrameSvg.AllBorders
            status: KSvg.Svg.Selected
        }
    )"),
                 QUrl(__FILE__));
    QVERIFY(!app.hasError());
    QCOMPARE(app.rootObjects().count(), 1);
}

// Note: Keep this test conceptually in sync with SvgItemTest::tst_implicitSize
void FrameSvgItemTest::tst_implicitSize()
{
    // Create a framesvgitem
    // assign one image
    // check implicit size
    // assign another prefix to it
    // check implicit size again
    // assign another image to it
    // check implicit size again
    // override width
    // check implicit width
    // override height
    // check implicit height
    // assign first image
    // check implicit size (should stay the same)

    QString pathToBackground = QStringLiteral("qrc:/data/background.svg");
    QString pathToSlider = QStringLiteral("qrc:/data/slider.svg");

    QQmlApplicationEngine app;
    app.setInitialProperties({{QStringLiteral("imagePath"), pathToBackground}});
    app.loadData(QByteArrayLiteral(R"(
        import org.kde.ksvg as KSvg

        KSvg.FrameSvgItem {
            prefix: "prefix"
        }
    )"),
                 QUrl(__FILE__));
    QVERIFY(!app.hasError());
    QCOMPARE(app.rootObjects().count(), 1);

    QQuickItem *frameSvgItem = qobject_cast<QQuickItem *>(app.rootObjects().first());
    QVERIFY(frameSvgItem);

    const QObject *margins = frameSvgItem->property("margins").value<QObject *>();
    QVERIFY(margins && margins->inherits("KSvg::FrameSvgItemMargins"));

    QSignalSpy implicitWidthSpy(frameSvgItem, SIGNAL(implicitWidthChanged()));
    QSignalSpy implicitHeightSpy(frameSvgItem, SIGNAL(implicitHeightChanged()));
    QSignalSpy marginsChangedSpy(margins, SIGNAL(marginsChanged()));
    QVERIFY(implicitWidthSpy.isValid());
    QVERIFY(implicitHeightSpy.isValid());
    QVERIFY(marginsChangedSpy.isValid());

    QCOMPARE(frameSvgItem->property("usedPrefix"), QStringLiteral("prefix"));

    QCOMPARE(margins->property("top").toReal(), 5.0);
    QCOMPARE(margins->property("left").toReal(), 5.0);
    QCOMPARE(margins->property("right").toReal(), 5.0);
    QCOMPARE(margins->property("bottom").toReal(), 5.0);

    QCOMPARE(frameSvgItem->implicitWidth(), 10.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 10.0);

    frameSvgItem->setProperty("prefix", QStringList());
    QCOMPARE(frameSvgItem->property("usedPrefix"), QString());

    QCOMPARE(implicitWidthSpy.count(), 1);
    QCOMPARE(implicitHeightSpy.count(), 1);
    QCOMPARE(marginsChangedSpy.count(), 1);

    QCOMPARE(margins->property("top").toReal(), 26.0);
    QCOMPARE(margins->property("left").toReal(), 26.0);
    QCOMPARE(margins->property("right").toReal(), 26.0);
    QCOMPARE(margins->property("bottom").toReal(), 26.0);

    QCOMPARE(frameSvgItem->implicitWidth(), 52.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 52.0);

    frameSvgItem->setProperty("imagePath", pathToSlider);
    frameSvgItem->setProperty("prefix", QStringLiteral("groove"));
    QCOMPARE(frameSvgItem->property("usedPrefix"), QStringLiteral("groove"));

    QCOMPARE(implicitWidthSpy.count(), 3);
    QCOMPARE(implicitHeightSpy.count(), 3);
    QCOMPARE(marginsChangedSpy.count(), 3);

    QCOMPARE(margins->property("top").toReal(), 3.0);
    QCOMPARE(margins->property("left").toReal(), 3.0);
    QCOMPARE(margins->property("right").toReal(), 3.0);
    QCOMPARE(margins->property("bottom").toReal(), 3.0);

    QCOMPARE(frameSvgItem->implicitWidth(), 6.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 6.0);

    frameSvgItem->setImplicitWidth(100.0);
    QCOMPARE(implicitWidthSpy.count(), 4);

    frameSvgItem->setProperty("prefix", QStringLiteral("groove-highlight"));
    QCOMPARE(frameSvgItem->property("usedPrefix"), QStringLiteral("groove-highlight"));

    QCOMPARE(implicitWidthSpy.count(), 4);
    QCOMPARE(implicitHeightSpy.count(), 3);

    QCOMPARE(frameSvgItem->implicitWidth(), 100.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 6.0);

    frameSvgItem->setImplicitHeight(200.0);

    frameSvgItem->setProperty("imagePath", pathToBackground);
    frameSvgItem->setProperty("prefix", QVariant());

    QCOMPARE(frameSvgItem->implicitWidth(), 100.0);
    QCOMPARE(frameSvgItem->implicitHeight(), 200.0);
}

QTEST_MAIN(FrameSvgItemTest)

#include "framesvgitemtest.moc"
