// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 SonicDE contributors

#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QNetworkProxy>
#include <QProcess>
#include <QQmlApplicationEngine>
#include <QQmlIncubationController>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include <cstdlib>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include "backtracegenerator.h"
#include "debuggermanager.h"
#include "drkonqi.h"
#include "drkonqidialog.h"
#include "qmlextensions/doctore.h"
#include "settings.h"

static void initializeCrash(bool safer, bool restarted, bool ownCrash = false, bool hasBugAddress = true)
{
    Settings::self()->setSentry(false);
    DrKonqi::setPid(0); // Populate the models without attaching to or stopping any process.
    DrKonqi::setAppName(ownCrash ? QStringLiteral("drkonqi") : QStringLiteral("sample-app"));
    DrKonqi::setAppPath(QStringLiteral("/nonexistent"));
    DrKonqi::setProgramName(QStringLiteral("Sample Application"));
    DrKonqi::setBugAddress(hasBugAddress ? QStringLiteral("submit@bugs.kde.org") : QString());
    DrKonqi::setSignal(11);
    DrKonqi::setSafer(safer);
    DrKonqi::setRestarted(restarted);
    // KCrashBackend constructs its models before rejecting PID 0. Do not run a
    // debugger or generate real crash events in this interface test.
    if (DrKonqi::init()) {
        qFatal("The interface test unexpectedly initialized a real crash backend.");
    }
    DrKonqi::debuggerManager()->backtraceGenerator()->setBackendFailed();
}

class DrKonqiDialogTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testDialog_data()
    {
        QTest::addColumn<bool>("safer");
        QTest::addColumn<bool>("minimal");
        QTest::addColumn<bool>("onlyBacktrace");
        QTest::addColumn<bool>("restarted");
        QTest::addColumn<bool>("ownCrash");
        QTest::addColumn<bool>("hasBugAddress");
        QTest::addColumn<bool>("sentryPage");

        QTest::newRow("normal") << false << false << false << false << false << true << false;
        QTest::newRow("safer") << true << false << false << false << false << true << false;
        QTest::newRow("minimal") << false << true << false << false << false << true << false;
        QTest::newRow("backtrace-only") << false << false << true << false << false << true << false;
        QTest::newRow("safer-backtrace") << true << false << true << false << false << true << false;
        QTest::newRow("minimal-backtrace") << false << true << true << false << false << true << false;
        QTest::newRow("already-restarted") << false << true << false << true << false << true << false;
        QTest::newRow("own-crash") << false << false << false << false << true << true << false;
        QTest::newRow("own-safer") << true << false << false << false << true << true << false;
        QTest::newRow("no-bug-address") << false << false << false << false << false << false << false;
        QTest::newRow("sentry") << false << false << false << false << false << true << true;
        QTest::newRow("safer-sentry") << true << false << false << false << false << true << true;
        QTest::newRow("minimal-sentry") << false << true << false << false << false << true << true;
    }

    void testDialog()
    {
        QFETCH(bool, safer);
        QFETCH(bool, minimal);
        QFETCH(bool, onlyBacktrace);
        QFETCH(bool, restarted);
        QFETCH(bool, ownCrash);
        QFETCH(bool, hasBugAddress);
        QFETCH(bool, sentryPage);

        qputenv("DRKONQI_MINIMAL_MODE", minimal ? "1" : "0");
        KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("General"));
        config.writeEntry(QStringLiteral("ShowOnlyBacktrace"), onlyBacktrace);
        initializeCrash(safer, restarted, ownCrash, hasBugAddress);

        DrKonqiDialog dialog;
        dialog.show(sentryPage ? DrKonqiDialog::GoTo::Sentry : DrKonqiDialog::GoTo::Main);
        auto* engine = dialog.findChild<QQmlApplicationEngine*>();
        QVERIFY(engine);
        QTRY_COMPARE(engine->rootObjects().size(), 1);
        auto* window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
        QVERIFY(window); // Every mode uses the existing QML window, never a QDialog.
        QVERIFY(QApplication::topLevelWidgets().isEmpty());
        QTRY_VERIFY(window->isVisible());
        QVERIFY(QTest::qWaitForWindowExposed(window));
        if (auto* controller = engine->incubationController()) {
            QTRY_COMPARE(controller->incubatingObjectCount(), 0);
        }
        QCOMPARE(window->property("minimalMode").toBool(), minimal);
        QCOMPARE(window->property("showOnlyBacktrace").toBool(), onlyBacktrace);
        QCOMPARE(window->property("restrictedMode").toBool(), safer || minimal);
        QCOMPARE(window->property("canReport").toBool(), hasBugAddress && !safer && !minimal && !ownCrash);
        QCOMPARE(window->property("canAutoReport").toBool(), hasBugAddress && !safer && !minimal);

        auto* stack = window->property("pageStack").value<QObject*>();
        QVERIFY(stack);
        QTRY_VERIFY(stack->property("currentItem").value<QObject*>());
        const auto expectedTitle = onlyBacktrace ? QStringLiteral("Developer Information")
            : sentryPage && !safer && !minimal   ? QStringLiteral("Automatic Report")
                                                 : QStringLiteral("Crash Handler");
        QTRY_COMPARE(stack->property("currentItem").value<QObject*>()->property("title").toString(), expectedTitle);

        auto* restart = window->findChild<QObject*>(QStringLiteral("restartApplicationAction"));
        QVERIFY(restart);
        QCOMPARE(restart->property("enabled").toBool(), !restarted && !ownCrash);
        QVERIFY(window->findChild<QObject*>(QStringLiteral("closeDialogAction")));
        Doctore::copyToClipboard(QStringLiteral("  test backtrace\n"));
        QCOMPARE(QApplication::clipboard()->text(), QStringLiteral("test backtrace"));

        if (safer || minimal) {
            auto* page = stack->property("currentItem").value<QObject*>();
            QVERIFY(QMetaObject::invokeMethod(window, "goToSentry"));
            QCOMPARE(stack->property("currentItem").value<QObject*>(), page);
        }

        const QString screenshots = qEnvironmentVariable("DRKONQI_UI_SCREENSHOTS");
        if (!screenshots.isEmpty()) {
            QVERIFY(QDir().mkpath(screenshots));
            QTRY_VERIFY(window->isExposed());
            const auto image = window->grabWindow();
            QVERIFY(!image.isNull());
            QVERIFY(image.save(screenshots + QLatin1Char('/') + QString::fromLatin1(QTest::currentDataTag()) + QStringLiteral(".png")));
        }
        QVERIFY(window->close());
    }

    void testLoadFailure()
    {
        QProcess process;
        process.start(QCoreApplication::applicationFilePath(), {QStringLiteral("--qml-load-failure")});
        QVERIFY(process.waitForFinished(10000));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QCOMPARE(process.exitCode(), EXIT_FAILURE);
        QVERIFY(process.readAllStandardError().contains("Failed to load the QML crash dialog."));
    }
};

int main(int argc, char** argv)
{
    QTemporaryDir home;
    if (!home.isValid()) {
        return EXIT_FAILURE;
    }
    qputenv("HOME", home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", home.path().toUtf8());
    qputenv("XDG_DATA_HOME", home.path().toUtf8());
    qputenv("XDG_CACHE_HOME", home.path().toUtf8());
    qputenv("DRKONQI_KDE_BUGZILLA_URL", "http://127.0.0.1:9/"); // No external network in tests.
    qputenv("LANGUAGE", "en_US");
    QStandardPaths::setTestModeEnabled(true);
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("drkonqi-dialog-test"));
    app.setQuitOnLastWindowClosed(false);
    // Sentry also looks up its endpoint configuration independently of the
    // reporting preference. Keep all such requests inside the test machine.
    QNetworkProxy::setApplicationProxy(QNetworkProxy(QNetworkProxy::HttpProxy, QStringLiteral("127.0.0.1"), 9));
    KLocalizedString::setApplicationDomain("drkonqi");
    if (app.arguments().contains(QStringLiteral("--qml-load-failure"))) {
        initializeCrash(false, false);
        Q_CLEANUP_RESOURCE(qml);
        DrKonqiDialog dialog;
        dialog.show(DrKonqiDialog::GoTo::Main);
        return app.exec(); // Must exit nonzero rather than leave an invisible process.
    }
    DrKonqiDialogTest test;
    return QTest::qExec(&test, argc, argv);
}

#include "drkonqidialogtest.moc"
