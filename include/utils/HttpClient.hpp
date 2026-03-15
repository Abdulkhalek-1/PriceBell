#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <functional>

// Thin async wrapper around QNetworkAccessManager.
// Caller provides a callback; the reply is parsed on the main thread via Qt event loop.
class HttpClient : public QObject {
    Q_OBJECT
public:
    using Callback = std::function<void(bool success, const QString& body, const QString& error)>;

    explicit HttpClient(QObject* parent = nullptr);

    // Issues a GET request. Calls callback(success, responseBody, errorMessage) when done.
    void get(const QString& url, Callback callback);

    // Sets request headers (e.g. Authorization, User-Agent).
    void setHeader(const QString& key, const QString& value);

private:
    QNetworkAccessManager* m_manager;
    QMap<QString, QString> m_headers;
};
