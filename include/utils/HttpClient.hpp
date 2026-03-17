#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QMap>
#include <functional>
#include "utils/Constants.hpp"

// Result of a synchronous HTTP request.
struct SyncResponse {
    bool ok = false;
    QByteArray body;
    QString error;
};

// Thin wrapper around QNetworkAccessManager with both async and sync APIs.
// Caller provides a callback for async; sync methods block via QEventLoop with timeout.
class HttpClient : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(bool success, const QString& body, const QString& error)>;

    explicit HttpClient(QObject* parent = nullptr);

    // Issues an async GET request. Calls callback(success, responseBody, errorMessage) when done.
    void get(const QString& url, Callback callback);

    // Synchronous GET with timeout.
    SyncResponse getSync(const QUrl& url, int timeoutMs = PriceBell::kDefaultNetworkTimeoutMs);

    // Synchronous POST with timeout.
    SyncResponse postSync(const QUrl& url, const QByteArray& body,
                          const QMap<QString, QString>& headers = {},
                          int timeoutMs = PriceBell::kDefaultNetworkTimeoutMs);

    // Sets default request headers (e.g. Authorization, User-Agent).
    void setHeader(const QString& key, const QString& value);

private:
    QNetworkAccessManager* m_manager;
    QMap<QString, QString> m_headers;
};
