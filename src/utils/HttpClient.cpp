#include "utils/HttpClient.hpp"
#include "utils/Logger.hpp"

#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>

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

SyncResponse HttpClient::getSync(const QUrl& url, int timeoutMs) {
    SyncResponse response;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());

    QEventLoop loop;
    QNetworkReply* reply = m_manager->get(request);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        response.error = "Request timed out";
        reply->deleteLater();
        return response;
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        response.error = reply->errorString();
    } else {
        response.ok = true;
        response.body = reply->readAll();
    }
    reply->deleteLater();
    return response;
}

SyncResponse HttpClient::postSync(const QUrl& url, const QByteArray& body,
                                   const QMap<QString, QString>& headers,
                                   int timeoutMs) {
    SyncResponse response;
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    for (auto it = m_headers.begin(); it != m_headers.end(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    for (auto it = headers.begin(); it != headers.end(); ++it)
        request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());

    QEventLoop loop;
    QNetworkReply* reply = m_manager->post(request, body);

    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    if (!timer.isActive()) {
        reply->abort();
        response.error = "Request timed out";
        reply->deleteLater();
        return response;
    }
    timer.stop();

    if (reply->error() != QNetworkReply::NoError) {
        response.error = reply->errorString();
    } else {
        response.ok = true;
        response.body = reply->readAll();
    }
    reply->deleteLater();
    return response;
}
