#include "utils/HttpClient.hpp"
#include "utils/Logger.hpp"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{}

void HttpClient::setHeader(const QString& key, const QString& value) {
    m_headers[key] = value;
}

void HttpClient::get(const QString& url, Callback callback) {
    QUrl _url(url); QNetworkRequest request{_url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply = m_manager->get(request);
    connect(reply, &QNetworkReply::finished, [reply, callback]() {
        if (reply->error() != QNetworkReply::NoError) {
            callback(false, QString(), reply->errorString());
        } else {
            callback(true, QString::fromUtf8(reply->readAll()), QString());
        }
        reply->deleteLater();
    });
}
