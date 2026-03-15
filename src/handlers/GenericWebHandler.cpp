#include "handlers/GenericWebHandler.hpp"
#include "utils/Logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <sstream>

GenericWebHandler::GenericWebHandler(const SourceConfig& config, HttpClient* http)
    : m_config(config)
    , m_http(http)
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

bool GenericWebHandler::validateUrl(const std::string& url) const {
    return url.find("https://") == 0 || url.find("http://") == 0;
}

FetchResult GenericWebHandler::fetchProduct(const std::string& url) {
    if (!validateUrl(url)) {
        return FetchResult{false, 0.0f, 0.0f, "Invalid URL for this handler"};
    }

    FetchResult result;

    // Build request URL: replace {url} template placeholder if present
    QString requestUrl = QString::fromStdString(m_config.urlTemplate.empty() ? url : m_config.urlTemplate);
    requestUrl.replace("{url}", QString::fromStdString(url));

    auto resp = m_http->getSync(QUrl(requestUrl));
    if (!resp.ok) {
        result.errorMsg = resp.error.toStdString();
        Logger::error("GenericWebHandler fetch error [" + m_config.id + "]: " + result.errorMsg);
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(resp.body);
    if (doc.isNull()) {
        return FetchResult{false, 0.0f, 0.0f, "Invalid JSON response"};
    }

    std::string body = resp.body.toStdString();
    result.price    = extractJsonPath(body, m_config.pricePath);
    result.discount = extractJsonPath(body, m_config.discountPath);
    result.success  = true;

    Logger::info("GenericWebHandler [" + m_config.id + "]: price=" + std::to_string(result.price));
    return result;
}
