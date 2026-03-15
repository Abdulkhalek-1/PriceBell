#include "handlers/SteamHandler.hpp"
#include "utils/Logger.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QUrl>
#include <regex>

// Steam Store API endpoint
static const char* kSteamApiBase =
    "https://store.steampowered.com/api/appdetails?appids=%s&cc=us&filters=price_overview";

SteamHandler::SteamHandler(HttpClient* http)
    : m_http(http)
{}

std::string SteamHandler::extractAppId(const std::string& url) {
    // Match patterns like /app/730/ or /app/730
    std::regex re(R"(/app/(\d+))");
    std::smatch m;
    if (std::regex_search(url, m, re) && m.size() > 1)
        return m[1].str();
    return {};
}

bool SteamHandler::validateUrl(const std::string& url) const {
    // Accept any store.steampowered.com URL that contains /app/<id>
    // Covers /app/730/, /agecheck/app/730/, etc.
    return (url.find("https://store.steampowered.com/") == 0
         || url.find("http://store.steampowered.com/") == 0)
        && url.find("/app/") != std::string::npos;
}

FetchResult SteamHandler::fetchProduct(const std::string& url) {
    if (!validateUrl(url)) {
        return FetchResult{false, 0.0f, 0.0f, "Invalid URL for this handler"};
    }

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

    auto resp = m_http->getSync(QUrl(QString::fromStdString(apiUrl)));
    if (!resp.ok) {
        result.errorMsg = resp.error.toStdString();
        Logger::error("Steam fetch error: " + result.errorMsg);
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(resp.body);

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
