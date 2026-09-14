// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2009 Dario Andres Rodriguez <andresbajotierra@gmail.com>
// SPDX-FileCopyrightText: 2022 Harald Sitter <sitter@kde.org>

#include "drkonqidialog.h"

#include <KConfigGroup>
#include <KLocalizedString>
#include <KSharedConfig>

#include <QCoreApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include <cstdlib>

#include "backtracegenerator.h"
#include "backtraceparser.h"
#include "bugzillaintegration/bugzillalib.h"
#include "config-drkonqi.h"
#include "crashedapplication.h"
#include "debuggermanager.h"
#include "debugpackageinstaller.h"
#include "drkonqi.h"
#include "qmlextensions/credentialstore.h"
#include "qmlextensions/doctore.h"
#include "qmlextensions/platformmodel.h"
#include "qmlextensions/reproducibilitymodel.h"
#include "settings.h"

void DrKonqiDialog::show(DrKonqiDialog::GoTo to)
{
    auto engine = new QQmlApplicationEngine(this);

    auto l10nContext = new KLocalizedContext(engine);
    l10nContext->setTranslationDomain(QStringLiteral(TRANSLATION_DOMAIN));
    engine->rootContext()->setContextObject(l10nContext);

    qmlRegisterType<BugzillaManager>("org.kde.drkonqi", 1, 0, "Bugzilla");
    qmlRegisterType<PlatformModel>("org.kde.drkonqi", 1, 0, "PlatformModel");
    qmlRegisterType<ReproducibilityModel>("org.kde.drkonqi", 1, 0, "ReproducibilityModel");
    qmlRegisterType<CredentialStore>("org.kde.drkonqi", 1, 0, "CredentialStore");
    qmlRegisterType<DebugPackageInstaller>("org.kde.drkonqi", 1, 0, "DebugPackageInstaller");

    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "ReportInterface", ReportInterface::self());
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "CrashedApplication", DrKonqi::crashedApplication());
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "BacktraceGenerator", DrKonqi::debuggerManager()->backtraceGenerator());

    static Doctore doctore;
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "DrKonqi", &doctore);

    auto settings = Settings::self();
    qmlRegisterSingletonInstance("org.kde.drkonqi", 1, 0, "Settings", settings);

    // TODO do we need this second BG?
    qmlRegisterUncreatableType<BacktraceGenerator>("org.kde.drkonqi", 1, 0, "BacktraceGenerator1", QStringLiteral("Cannot create WarningLevel in QML"));
    qmlRegisterUncreatableType<BacktraceParser>("org.kde.drkonqi", 1, 0, "BacktraceParser", QStringLiteral("Cannot create WarningLevel in QML"));

    const QUrl mainUrl(QStringLiteral("qrc:/ui/main.qml"));
    const KConfigGroup config(KSharedConfig::openConfig(), QStringLiteral("General"));
    engine->setInitialProperties({{QStringLiteral("minimalMode"), DrKonqi::minimalMode()},
        {QStringLiteral("showOnlyBacktrace"), config.readEntry(QStringLiteral("ShowOnlyBacktrace"), false)}});
    QObject::connect(
        engine,
        &QQmlApplicationEngine::objectCreationFailed,
        QCoreApplication::instance(),
        [] {
            qCritical() << "Failed to load the QML crash dialog.";
            QCoreApplication::exit(EXIT_FAILURE);
        },
        Qt::QueuedConnection);
    QObject::connect(
        engine,
        &QQmlApplicationEngine::objectCreated,
        this,
        [mainUrl, to](QObject* obj, const QUrl& objUrl) {
            if (!obj || objUrl != mainUrl) {
                return;
            }

            switch (to) {
            case GoTo::Main:
                break;
            case GoTo::Sentry:
                QMetaObject::invokeMethod(obj, "goToSentry", Qt::QueuedConnection);
                break;
            }
        },
        Qt::QueuedConnection);
    engine->load(mainUrl);
}

#include "moc_drkonqidialog.cpp"
