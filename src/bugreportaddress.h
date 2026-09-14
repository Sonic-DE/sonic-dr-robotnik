/*
    SPDX-FileCopyrightText: 2009 George Kiagiadakis <gkiagia@users.sourceforge.net>
    SPDX-FileCopyrightText: 2026 Harald Sitter <sitter@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef BUGREPORTADDRESS_H
#define BUGREPORTADDRESS_H

#include <QString>
#include <QUrl>

#include "drkonqi_debug.h"
#include "drkonqi_globals.h"

class BugReportAddress : public QString {
public:
    inline BugReportAddress()
        : QString()
    {
    }
    inline BugReportAddress(const QString& address)
        : QString(looksLikeKDE(address) ? KDE_BUGZILLA_URL : address)
    {
    }

    inline bool isKdeBugzilla() const
    {
        return *this == KDE_BUGZILLA_URL;
    }

    inline bool isEmail() const
    {
        return contains(QLatin1Char('@'));
    }

private:
    [[nodiscard]] static bool looksLikeKDE(const QString& address)
    {
        // The default value is a raw email address from the days of yore. Special case it since it is not a URI.
        if (address == QLatin1String("submit@bugs.kde.org")) {
            return true;
        }
        if (address.isEmpty()) {
            return false;
        }

        QUrl url(address, QUrl::StrictMode);
        if (!url.isValid()) {
            qCWarning(DRKONQI_LOG) << "Invalid URI" << address;
            return false;
        }

        QString host;
        if (url.scheme() == QLatin1String("mailto")) {
            // A mailto URI has a mailbox in its path, not an authority. Only
            // normalize a single recipient; mixed destinations must stay intact.
            const QString mailbox = url.path();
            const auto separator = mailbox.indexOf(QLatin1Char('@'));
            if (!url.authority().isEmpty() || separator <= 0 || separator != mailbox.lastIndexOf(QLatin1Char('@'))) {
                return false;
            }
            QUrl recipient;
            // path() is decoded, so do not interpret percent escapes a second time.
            recipient.setHost(mailbox.mid(separator + 1), QUrl::DecodedMode);
            if (!recipient.isValid()) {
                return false;
            }
            host = recipient.host();
        } else if (url.scheme() == QLatin1String("https") || url.scheme() == QLatin1String("http")) {
            host = url.host();
        } else {
            return false;
        }
        // Cover all bases here: bugs.kde.org, bugstest.kde.org, invent.kde.org...
        return host.endsWith(QLatin1String(".kde.org"));
    }
};

#endif
