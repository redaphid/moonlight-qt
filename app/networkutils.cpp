#include "networkutils.h"

void NetworkUtils::ensureNetworkAccessibility(QNetworkAccessManager* nam)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0) && QT_VERSION < QT_VERSION_CHECK(5, 15, 1) && !defined(QT_NO_BEARERMANAGEMENT)
    // HACK: Set network accessibility to work around QTBUG-80947 (introduced in Qt 5.14.0 and fixed in Qt 5.15.1)
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_DEPRECATED
    nam->setNetworkAccessible(QNetworkAccessManager::Accessible);
    QT_WARNING_POP
#else
    Q_UNUSED(nam);
#endif
}
