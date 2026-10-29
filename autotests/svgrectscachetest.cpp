/*
    SPDX-FileCopyrightText: 2026 KDE Contributors

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QImage>
#include <QStandardPaths>
#include <QTest>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KSvg/Svg>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

// SvgRectsCache is a process-global singleton holding one KSharedConfig
// ("~/.cache/ksvg-elements") plus several QHash members, all without locking.
// Svg::setImagePath()/image() reach it from multiple threads (main + render),
// so concurrent access corrupts the KEntryMap (and the hashes) and eventually
// aborts in malloc() - bug 506642.
class SvgRectsCacheTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

    void testConcurrentCacheAccess()
    {
        const QString path = QFINDTESTDATA("data/background.svgz");
        QVERIFY2(!path.isEmpty(), "test SVG not found");

        const QString cacheFile = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/ksvg-elements");

        std::atomic_bool stop{false};
        std::atomic<quint64> iterations{0};

        // Corruptor: Svg::image() reaches the unlocked SvgRectsCache from
        // several threads and mutates its KSharedConfig.
        auto worker = [&]() {
            KSvg::Svg svg;
            // Only exercise SvgRectsCache here; the ImageSet pixmap cache is a
            // different, unrelated global cache that would otherwise race too.
            svg.setUsingRenderingCache(false);
            svg.setImagePath(path);
            if (!svg.isValid()) {
                return;
            }
            while (!stop.load(std::memory_order_relaxed)) {
                (void)svg.image(QSize(64, 64));
                (void)svg.image(QSize(32, 32));
                iterations.fetch_add(1, std::memory_order_relaxed);
            }
        };

        // Victim: keeps allocating through KConfig. Once the corruptor has
        // damaged the heap, this is where the abort usually shows up, giving
        // the same kind of backtrace as bug 506642.
        auto victim = [&]() {
            while (!stop.load(std::memory_order_relaxed)) {
                KSharedConfig::Ptr config = KSharedConfig::openConfig(cacheFile, KConfig::SimpleConfig);
                config->reparseConfiguration();
                (void)config->group(QStringLiteral("General")).readEntry(QStringLiteral("IconThemePath"), QString());
            }
        };

        std::vector<std::thread> threads;
        for (int i = 0; i < 6; ++i) {
            threads.emplace_back(worker);
        }
        for (int i = 0; i < 3; ++i) {
            threads.emplace_back(victim);
        }

        std::this_thread::sleep_for(std::chrono::seconds(10));
        stop.store(true, std::memory_order_relaxed);
        for (auto &thread : threads) {
            thread.join();
        }

        qInfo() << "completed" << iterations.load() << "concurrent Svg image requests";
    }
};

QTEST_MAIN(SvgRectsCacheTest)

#include "svgrectscachetest.moc"
