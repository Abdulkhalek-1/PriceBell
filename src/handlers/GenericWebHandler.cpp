#include "handlers/GenericWebHandler.hpp"
#include "utils/Logger.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QEventLoop>
#include <sstream>

GenericWebHandler::GenericWebHandler(const SourceConfig& config)
    : m_config(config)
{}

// Simple dot-notation JSON path extractor.
// e.g. "data.price.amount" traverses nested objects and returns the float value.
float GenericWebHandler::extractJsonPath(const std::string& json, const std::string& path) {
    if (path.empty()) return 0.0f;

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    QJsonValue current = doc.object();

    std::istringstream ss(path);
    std::string segment;
    while (std::getline(ss, segment, '.')) {
        if (current.isObject()) {
            current = current.toObject().value(QString::fromStdString(segment));
        } else {
            return 0.0f;
        }
    }

    if (current.isDouble()) return static_cast<float>(current.toDouble());
    if (current.isString()) return current.toString().toFloat();
    return 0.0f;
}

FetchResult GenericWebHandler::fetchProduct(const std::string& url) {
    FetchResult result;

    // Build request URL: replace {url} template placeholder if present
    QString requestUrl = QString::fromStdString(m_config.urlTemplate.empty() ? url : m_config.urlTemplate);
    requestUrl.replace("{url}", QString::fromStdString(url));

    QNetworkAccessManager mgr;
    QUrl _url(requestUrl); QNetworkRequest request{_url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");

    QEventLoop loop;
    QNetworkReply* reply = mgr.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        result.errorMsg = reply->errorString().toStdString();
        Logger::error("GenericWebHandler fetch error [" + m_config.id + "]: " + result.errorMsg);
        reply->deleteLater();
        return result;
    }

    std::string body = reply->readAll().toStdString();
    reply->deleteLater();

    result.price    = extractJsonPath(body, m_config.pricePath);
    result.discount = extractJsonPath(body, m_config.discountPath);
    result.success  = true;

    Logger::info("GenericWebHandler [" + m_config.id + "]: price=" + std::to_string(result.price));
    return result;
}
