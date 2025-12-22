#pragma once

#include <QNetworkAccessManager>

namespace NetworkUtils {
    // Workaround for QTBUG-80947 (introduced in Qt 5.14.0 and fixed in Qt 5.15.1)
    // https://bugreports.qt.io/browse/QTBUG-80947
    //
    // This ensures network accessibility is properly set for QNetworkAccessManager
    // on affected Qt versions to avoid network requests failing to start.
    void ensureNetworkAccessibility(QNetworkAccessManager* nam);
}
