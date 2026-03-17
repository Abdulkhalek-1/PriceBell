#include "handlers/UdemyHandler.hpp"
#include "utils/Logger.hpp"
#include "utils/SettingsProvider.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <regex>

// Udemy API endpoint
// Docs: https://www.udemy.com/developers/affiliate/
static const char* kUdemyApiBase =
    "https://www.udemy.com/api-2.0/courses/%s/?fields[course]=price_detail,discount";

UdemyHandler::UdemyHandler(HttpClient* http)
    : m_http(http)
{}

std::string UdemyHandler::extractCourseId(const std::string& url) {
    // Match /course/<slug>/ and use slug as identifier
    // Udemy API accepts slug or numeric ID in the same endpoint
    std::regex re(R"(/course/([^/]+)/?(?:\?|$|#))");
    std::smatch m;
    if (std::regex_search(url, m, re) && m.size() > 1)
        return m[1].str();
    return {};
}

bool UdemyHandler::validateUrl(const std::string& url) const {
    return url.find("https://www.udemy.com/course/") == 0
        || url.find("http://www.udemy.com/course/") == 0
        || url.find("https://udemy.com/course/") == 0
        || url.find("http://udemy.com/course/") == 0;
}

FetchResult UdemyHandler::fetchProduct(const std::string& url) {
    if (!validateUrl(url)) {
        return FetchResult{false, 0.0f, 0.0f, "Invalid URL for this handler"};
    }

    FetchResult result;

    std::string courseId = extractCourseId(url);
    if (courseId.empty()) {
        result.errorMsg = "Could not extract Udemy course ID from URL: " + url;
        Logger::warn(result.errorMsg);
        return result;
    }

    // Read credentials from SettingsProvider
    QString clientId     = SettingsProvider::instance().udemyClientId();
    QString clientSecret = SettingsProvider::instance().udemyClientSecret();

    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        result.errorMsg = "Udemy API credentials not configured. Set them in Settings.";
        Logger::warn(result.errorMsg);
        return result;
    }

    char apiUrl[512];
    std::snprintf(apiUrl, sizeof(apiUrl), kUdemyApiBase, courseId.c_str());

    // Set authorization header on the HttpClient before the request
    QString credentials = clientId + ":" + clientSecret;
    QString encoded     = "Basic " + QString::fromUtf8(credentials.toUtf8().toBase64());
    m_http->setHeader("Authorization", encoded);

    auto resp = m_http->getSync(QUrl(QString::fromStdString(apiUrl)));
    if (!resp.ok) {
        result.errorMsg = resp.error.toStdString();
        Logger::error("Udemy fetch error: " + result.errorMsg);
        return result;
    }

    QJsonDocument doc  = QJsonDocument::fromJson(resp.body);
    QJsonObject root   = doc.object();

    // price_detail.amount contains the current price
    QJsonObject priceDetail = root.value("price_detail").toObject();
    result.price = static_cast<float>(priceDetail.value("amount").toDouble());

    // discount_info if present
    QJsonObject discountInfo = root.value("discount").toObject();
    if (!discountInfo.isEmpty()) {
        double original = discountInfo.value("price").toObject().value("amount").toDouble();
        if (original > 0)
            result.discount = static_cast<float>((original - result.price) / original * 100.0);
    }

    result.success = true;
    Logger::info("Udemy: courseId=" + courseId +
                 " price=$" + std::to_string(result.price));
    return result;
}
