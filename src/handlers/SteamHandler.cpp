#include "handlers/SteamHandler.hpp"
#include "utils/Logger.hpp"
#include "utils/SettingsProvider.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocale>
#include <QString>
#include <QUrl>
#include <regex>

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

// Resolve the Steam country code: explicit setting wins, otherwise fall back to
// the system locale's country mapped to ISO-3166 alpha-2. Steam's `cc` parameter
// is case-insensitive but Steam returns lower-case in examples — we follow suit.
static QString resolveSteamCountryCode() {
    QString cc = SettingsProvider::instance().steamCountryCode().trimmed();
    if (!cc.isEmpty()) return cc.toLower();
    QString fromLocale = QLocale::countryToString(QLocale::system().country());
    // QLocale::system().name() is typically "en_US", "ar_EG", "ja_JP", etc.
    // We extract the country half rather than the (verbose) human name.
    QString name = QLocale::system().name(); // e.g. "ar_EG"
    int us = name.indexOf('_');
    if (us > 0 && us + 1 < name.size()) {
        return name.mid(us + 1, 2).toLower();
    }
    (void)fromLocale;
    return QStringLiteral("us");
}

FetchResult SteamHandler::fetchProduct(const std::string& url) {
    if (!validateUrl(url)) {
        return FetchResult{false, 0.0f, 0.0f, 0.0f, "Invalid URL for this handler", "", ""};
    }

    FetchResult result;

    std::string appId = extractAppId(url);
    if (appId.empty()) {
        result.errorMsg = "Could not extract Steam app ID from URL: " + url;
        Logger::warn(result.errorMsg);
        return result;
    }

    const QString cc = resolveSteamCountryCode();
    QString apiUrl = QString("https://store.steampowered.com/api/appdetails"
                             "?appids=%1&cc=%2&l=english&filters=price_overview,basic")
                         .arg(QString::fromStdString(appId), cc);

    auto resp = m_http->getSync(QUrl(apiUrl));
    if (!resp.ok) {
        result.errorMsg = resp.error.toStdString();
        Logger::error("Steam fetch error: " + result.errorMsg);
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(resp.body);

    // Response shape: { "<appid>": { "success": true, "data": { ... } } }
    QJsonObject root   = doc.object();
    QJsonObject appObj = root.value(QString::fromStdString(appId)).toObject();

    if (!appObj.value("success").toBool()) {
        result.errorMsg = "Steam API returned success=false for appId " + appId;
        Logger::warn(result.errorMsg);
        return result;
    }

    QJsonObject data = appObj.value("data").toObject();

    // Extract product name (basic filter). Used by ProductDialog auto-detect.
    if (data.contains("name")) {
        result.name = data.value("name").toString().toStdString();
    }

    QJsonObject price = data.value("price_overview").toObject();

    if (price.isEmpty()) {
        // Free to play, unreleased, or not available in this region.
        // Treat as success with price=0 so the row doesn't flash an error.
        result.success  = true;
        result.price    = 0.0f;
        result.discount = 0.0f;
        return result;
    }

    // Defensive parse: prefer `final` (post-discount), fall back to `initial`.
    int finalCents   = price.value("final").toInt(-1);
    int initialCents = price.value("initial").toInt(-1);

    if (finalCents >= 0) {
        result.price = finalCents / 100.0f;
    } else if (initialCents >= 0) {
        result.price = initialCents / 100.0f;
    } else {
        result.errorMsg = "Steam price_overview missing both final and initial for appId " + appId;
        Logger::warn(result.errorMsg);
        return result;
    }

    if (initialCents > 0 && initialCents != finalCents) {
        result.originalPrice = initialCents / 100.0f;
    }

    result.discount = static_cast<float>(price.value("discount_percent").toInt());
    result.currency = price.value("currency").toString().toStdString();
    result.success  = true;

    Logger::info("Steam: appId=" + appId +
                 " cc=" + cc.toStdString() +
                 " price=" + std::to_string(result.price) + " " + result.currency +
                 " discount=" + std::to_string(static_cast<int>(result.discount)) + "%");
    return result;
}
