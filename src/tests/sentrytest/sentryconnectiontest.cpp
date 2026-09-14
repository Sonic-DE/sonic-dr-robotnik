// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2026 SonicDE contributors

#include <QPointer>
#include <QSignalSpy>
#include <QTest>

#include "sentryconnection.h"

class TestNetworkReply : public QNetworkReply {
public:
    void abort() override
    {
    }

protected:
    qint64 readData(char*, qint64) override
    {
        return -1;
    }
};

class SentryConnectionTest : public QObject {
    Q_OBJECT

private Q_SLOTS:
    void testReplyDestroyedBeforeWrapper()
    {
        auto* networkReply = new TestNetworkReply;
        auto* wrapper = new SentryNetworkReply(networkReply);
        delete networkReply;
        // The manager can destroy its replies before the connection's QObject
        // destructor deletes the wrappers. Never call deleteLater on freed data.
        delete wrapper;
    }

    void testWrapperSchedulesReplyDeletion()
    {
        QPointer<TestNetworkReply> networkReply = new TestNetworkReply;
        auto* wrapper = new SentryNetworkReply(networkReply);
        delete wrapper;
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(networkReply.isNull());
    }

    void testConnectionDestroyedWithPendingReply()
    {
        QPointer<SentryReply> reply;
        {
            SentryNetworkConnection connection;
            reply = connection.get(QNetworkRequest(QUrl(QStringLiteral("data:text/plain,crash-information"))));
            QVERIFY(reply);
        }
        QVERIFY(reply.isNull());
    }

    void testConnectionDestroyedWithFinishedReply()
    {
        QPointer<SentryReply> reply;
        {
            SentryNetworkConnection connection;
            reply = connection.get(QNetworkRequest(QUrl(QStringLiteral("data:text/plain,crash-information"))));
            QSignalSpy finished(reply, &SentryReply::finished);
            QVERIFY(finished.wait());
            QCOMPARE(reply->error(), QNetworkReply::NoError);
            QCOMPARE(reply->readAll(), QByteArray("crash-information"));
        }
        QVERIFY(reply.isNull());
    }
};

QTEST_GUILESS_MAIN(SentryConnectionTest)

#include "sentryconnectiontest.moc"
