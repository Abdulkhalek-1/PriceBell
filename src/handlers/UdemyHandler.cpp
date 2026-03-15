#include "handlers/UdemyHandler.hpp"
#include "utils/Logger.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QEventLoop>
#include <QSettings>
#include <regex>

// Udemy API endpoint
// Docs: https://www.udemy.com/developers/affiliate/
static const char* kUdemyApiBase =
    "https://www.udemy.com/api-2.0/courses/%s/?fields[course]=price_detail,discount";

std::string UdemyHandler::extractCourseId(const std::string& url) {
    // Match /course/<slug>/ and use slug as identifier
    // Udemy API accepts slug or numeric ID in the same endpoint
    std::regex re(R"(/course/([^/]+)/?(?:\?|$|#))");
    std::smatch m;
    if (std::regex_search(url, m, re) && m.size() > 1)
        return m[1].str();
    return {};
}

FetchResult UdemyHandler::fetchProduct(const std::string& url) {
    FetchResult result;

    std::string courseId = extractCourseId(url);
    if (courseId.empty()) {
        result.errorMsg = "Could not extract Udemy course ID from URL: " + url;
        Logger::warn(result.errorMsg);
        return result;
    }

    // Read credentials from Qt settings
    QSettings settings("PriceBell", "PriceBell");
    QString clientId     = settings.value("udemy/client_id").toString();
    QString clientSecret = settings.value("udemy/client_secret").toString();

    if (clientId.isEmpty() || clientSecret.isEmpty()) {
        result.errorMsg = "Udemy API credentials not configured. Set them in Settings.";
        Logger::warn(result.errorMsg);
        return result;
    }

    char apiUrl[512];
    std::snprintf(apiUrl, sizeof(apiUrl), kUdemyApiBase, courseId.c_str());

    QNetworkAccessManager mgr;
    QUrl _url(QString::fromStdString(apiUrl)); QNetworkRequest request{_url};
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");

    // Udemy uses HTTP Basic Auth with client_id:client_secret
    QString credentials = clientId + ":" + clientSecret;
    QString encoded     = "Basic " + QString::fromUtf8(credentials.toUtf8().toBase64());
    request.setRawHeader("Authorization", encoded.toUtf8());

    QEventLoop loop;
    QNetworkReply* reply = mgr.get(request);
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        result.errorMsg = reply->errorString().toStdString();
        Logger::error("Udemy fetch error: " + result.errorMsg);
        reply->deleteLater();
        return result;
    }

    QJsonDocument doc  = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();
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
