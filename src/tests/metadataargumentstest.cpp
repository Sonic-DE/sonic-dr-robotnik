// SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL

#include "metadata.h"

#include <QTest>

class MetadataArgumentsTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void platform_data()
    {
        QTest::addColumn<QString>("platform");
        for (const auto& platform : {"offscreen", "minimal", "xcb", "wayland", ""}) {
            QTest::newRow(platform) << QString::fromLatin1(platform);
        }
    }

    void platform()
    {
        QFETCH(QString, platform);
        const QVariantHash metadata{{"platform", platform},
            {"exe", "/usr/lib/kf6/kioworker"},
            {"appname", "kioworker"},
            {"apppath", "/usr/lib/kf6"},
            {"pid", "264845"},
            {"signal", "11"},
            {"restarted", "true"}};
        auto expected = metadata;
        expected.remove("platform");
        // Compare without relying on QVariantHash's iteration order.
        auto actualArguments = Metadata::metadataArguments(metadata);
        auto expectedArguments = Metadata::metadataArguments(expected);
        actualArguments.sort();
        expectedArguments.sort();
        QCOMPARE(actualArguments, expectedArguments);
        const auto arguments = Metadata::metadataArguments(metadata);
        QVERIFY(!arguments.contains("--platform"));
        QVERIFY(!arguments.contains("--exe"));
        for (const auto& key : {"appname", "apppath", "pid", "signal"}) {
            const auto index = arguments.indexOf(QStringLiteral("--%1").arg(QString::fromLatin1(key)));
            QVERIFY(index >= 0);
            QVERIFY(index + 1 < arguments.size());
            QCOMPARE(arguments.at(index + 1), metadata.value(key).toString());
        }
        QVERIFY(arguments.contains("--restarted"));
        QVERIFY(!arguments.contains("true"));
    }

    void noMetadata()
    {
        QVERIFY(Metadata::metadataArguments({}).isEmpty());
    }

    void doNotRelaunchDrkonqi()
    {
        QTest::ignoreMessage(QtWarningMsg, "drkonqi crashed, we aren't going to invoke it again, we might be the reason it crashed :O");
        QVERIFY(Metadata::metadataArguments({{"exe", "/usr/lib/drkonqi"}, {"platform", "offscreen"}, {"pid", "123"}}).isEmpty());
    }
};

QTEST_GUILESS_MAIN(MetadataArgumentsTest)

#include "metadataargumentstest.moc"
