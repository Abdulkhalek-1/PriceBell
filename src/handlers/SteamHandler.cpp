#include "handlers/SteamHandler.hpp"
#include "utils/Logger.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <QEventLoop>
#include <regex>

// Steam Store API endpoint
static const char* kSteamApiBase =
    "https://store.steampowered.com/api/appdetails?appids=%s&cc=us&filters=price_overview";

std::string SteamHandler::extractAppId(const std::string& url) {
    // Match patterns like /app/730/ or /app/730
    std::regex re(R"(/app/(\d+))");
    std::smatch m;
    if (std::regex_search(url, m, re) && m.size() > 1)
        return m[1].str();
    return {};
}

FetchResult SteamHandler::fetchProduct(const std::string& url) {
    FetchResult result;

    std::string appId = extractAppId(url);
    if (appId.empty()) {
        result.errorMsg = "Could not extract Steam app ID from URL: " + url;
        Logger::warn(result.errorMsg);
        return result;
    }

    // Build API URL
    char apiUrl[256];
    std::snprintf(apiUrl, sizeof(apiUrl), kSteamApiBase, appId.c_str());

    // Synchronous fetch using QEventLoop (runs in background thread via PricePoller)
    QNetworkAccessManager mgr;
    QUrl _url(QString::fromStdString(apiUrl)); QNetworkRequest request{_url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");

    QEventLoop loop;
    QNetworkReply* reply = mgr.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        result.errorMsg = reply->errorString().toStdString();
        Logger::error("Steam fetch error: " + result.errorMsg);
        reply->deleteLater();
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

    // Response shape: { "<appid>": { "success": true, "data": { "price_overview": { ... } } } }
    QJsonObject root   = doc.object();
    QJsonObject appObj = root.value(QString::fromStdString(appId)).toObject();

    if (!appObj.value("success").toBool()) {
        result.errorMsg = "Steam API returned success=false for appId " + appId;
        Logger::warn(result.errorMsg);
        return result;
    }

    QJsonObject data  = appObj.value("data").toObject();
    QJsonObject price = data.value("price_overview").toObject();

    if (price.isEmpty()) {
        // Free to play or not available in region
        result.success  = true;
        result.price    = 0.0f;
        result.discount = 0.0f;
        return result;
    }

    // final is in cents (e.g. 4999 = $49.99)
    result.price    = price.value("final").toInt() / 100.0f;
    result.discount = static_cast<float>(price.value("discount_percent").toInt());
    result.success  = true;

    Logger::info("Steam: appId=" + appId +
                 " price=$" + std::to_string(result.price) +
                 " discount=" + std::to_string(static_cast<int>(result.discount)) + "%");
    return result;
}
