// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

#include <QTest>

#include <bugreportaddress.h>
#include <drkonqi.h>

using namespace Qt::StringLiterals;

class BugReportAddressTest : public QObject {
    Q_OBJECT
private Q_SLOTS:
    void testKDE()
    {
        QVERIFY(BugReportAddress(u"submit@bugs.kde.org"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"mailto:submit@bugs.kde.org"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"mailto:submit@bugs.kde.org?foo=bar"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"https://bugs.kde.org/enter_bug.cgi?product=plasma-systemmonitor"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"https://invent.kde.org/kde-linux/kde-linux"_s).isKdeBugzilla());
    }

    void testNotKDE()
    {
        QVERIFY(!BugReportAddress(u"submit@bugs.kde.org.uk"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"mailto:submit@bugs.kde.org.uk"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"mailto:submit@bugs.kde.org.uk?foo=bar"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"https://bugskde.org"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"https://bugs.kde.org.uk"_s).isKdeBugzilla());
        QVERIFY(!BugReportAddress(u"https://submit@bugs.kde.org.uk"_s).isKdeBugzilla());
    }

    void testNonKdeTargets_data()
    {
        QTest::addColumn<QString>("address");
        QTest::newRow("local-file") << u"file:///tmp/bugs.kde.org"_s;
        QTest::newRow("file-host") << u"file://bugs.kde.org/report"_s;
        QTest::newRow("relative-path") << u"reports/bugs.kde.org"_s;
        QTest::newRow("absolute-path") << u"/tmp/bugs.kde.org"_s;
        QTest::newRow("javascript") << u"javascript:bugs.kde.org"_s;
        QTest::newRow("data") << u"data:text/plain,bugs.kde.org"_s;
        QTest::newRow("https-without-host") << u"https:bugs.kde.org"_s;
        QTest::newRow("mailto-without-mailbox") << u"mailto:bugs.kde.org"_s;
        QTest::newRow("mailto-authority") << u"mailto://bugs.kde.org"_s;
        QTest::newRow("mailto-path") << u"mailto:someone@example.net/bugs.kde.org"_s;
        QTest::newRow("mailto-mixed-recipients") << u"mailto:someone@example.net,submit@bugs.kde.org"_s;
        QTest::newRow("mailto-domain-in-query") << u"mailto:someone@example.net?subject=bugs.kde.org"_s;
        QTest::newRow("web-domain-in-path") << u"https://example.net/bugs.kde.org"_s;
        QTest::newRow("web-domain-in-query") << u"https://example.net/?target=https://bugs.kde.org"_s;
        QTest::newRow("web-userinfo") << u"https://bugs.kde.org@example.net/"_s;
        QTest::newRow("web-domain-prefix") << u"https://bugs.kde.org.example.net/"_s;
        QTest::newRow("invalid-percent-encoding") << u"https://bugs.kde.org/%zz"_s;
        QTest::newRow("empty") << QString();
    }

    void testNonKdeTargets()
    {
        QFETCH(QString, address);
        const BugReportAddress reportAddress(address);
        QVERIFY(!reportAddress.isKdeBugzilla());
        QCOMPARE(static_cast<const QString&>(reportAddress), address);
    }

    void testDomainNormalization()
    {
        QVERIFY(BugReportAddress(u"https://BUGS.KDE.ORG/report"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"mailto:submit@BUGS.KDE.ORG"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"MAILTO:submit@bugs.kde.org?subject=crash"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"http://bugstest.kde.org/"_s).isKdeBugzilla());
        QVERIFY(BugReportAddress(u"mailto:submit%40bugs.kde.org"_s).isKdeBugzilla());
        QCOMPARE(static_cast<QString>(BugReportAddress(u"https://invent.kde.org/project/issues"_s)), DrKonqi::kdeBugzillaURL());
        QCOMPARE(static_cast<QString>(BugReportAddress(DrKonqi::kdeBugzillaURL())), DrKonqi::kdeBugzillaURL());
    }
};

QTEST_GUILESS_MAIN(BugReportAddressTest)

#include "bugreportaddresstest.moc"
