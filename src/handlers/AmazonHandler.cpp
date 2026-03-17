#include "handlers/AmazonHandler.hpp"
#include "utils/Logger.hpp"
#include "utils/SettingsProvider.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <regex>

// Amazon Product Advertising API 5.0 (PA API)
// Docs: https://webservices.amazon.com/paapi5/documentation/
static const char* kAmazonApiUrl  = "https://webservices.amazon.com/paapi5/getitems";
static const char* kAmazonRegion  = "us-east-1";
static const char* kAmazonService = "ProductAdvertisingAPI";

AmazonHandler::AmazonHandler(HttpClient* http)
    : m_http(http)
{}

std::string AmazonHandler::extractAsin(const std::string& url) {
    // Match /dp/XXXXXXXXXX or /gp/product/XXXXXXXXXX
    std::regex re(R"(/(?:dp|gp/product)/([A-Z0-9]{10}))");
    std::smatch m;
    if (std::regex_search(url, m, re) && m.size() > 1)
        return m[1].str();
    return {};
}

bool AmazonHandler::validateUrl(const std::string& url) const {
    QUrl qurl(QString::fromStdString(url));
    QString host = qurl.host();
    return qurl.scheme() == "https" &&
           (host.startsWith("www.amazon.") || host.startsWith("amazon."));
}

QByteArray AmazonHandler::buildPayload(const std::string& asin, const QString& partnerTag) {
    QString payload = QString(R"({
        "ItemIds": ["%1"],
        "Resources": ["Offers.Listings.Price", "Offers.Listings.SavingBasis"],
        "PartnerTag": "%2",
        "PartnerType": "Associates",
        "Marketplace": "www.amazon.com"
    })").arg(QString::fromStdString(asin)).arg(partnerTag);
    return payload.toUtf8();
}

QMap<QString, QString> AmazonHandler::signRequest(const QByteArray& payload,
                                                    const QString& accessKey,
                                                    const QString& secretKey) {
    QString dateTime  = QDateTime::currentDateTimeUtc().toString("yyyyMMddTHHmmssZ");
    QString datestamp = dateTime.left(8);
    QString host      = "webservices.amazon.com";
    QString target    = "com.amazon.paapi5.v1.ProductAdvertisingAPIv1.GetItems";

    // Canonical headers
    QString canonicalHeaders = QString("content-encoding:amz-1.0\n"
                                       "content-type:application/json; charset=utf-8\n"
                                       "host:%1\n"
                                       "x-amz-date:%2\n"
                                       "x-amz-target:%3\n").arg(host, dateTime, target);
    QString signedHeaders = "content-encoding;content-type;host;x-amz-date;x-amz-target";

    QByteArray payloadHash = QCryptographicHash::hash(
        payload, QCryptographicHash::Sha256).toHex();

    QString canonicalRequest = QString("POST\n/paapi5/getitems\n\n%1\n%2\n%3")
        .arg(canonicalHeaders, signedHeaders, QString(payloadHash));

    QString credentialScope = datestamp + "/" + kAmazonRegion + "/" + kAmazonService + "/aws4_request";
    QByteArray stringToSign = QString("AWS4-HMAC-SHA256\n%1\n%2\n%3")
        .arg(dateTime, credentialScope,
             QString(QCryptographicHash::hash(canonicalRequest.toUtf8(),
                                              QCryptographicHash::Sha256).toHex()))
        .toUtf8();

    auto hmac = [](const QByteArray& key, const QByteArray& data) {
        return QMessageAuthenticationCode::hash(data, key, QCryptographicHash::Sha256);
    };
    QByteArray signingKey = hmac(
        hmac(hmac(hmac("AWS4" + secretKey.toUtf8(), datestamp.toUtf8()),
                  QString(kAmazonRegion).toUtf8()),
             QString(kAmazonService).toUtf8()),
        "aws4_request");
    QString signature = hmac(signingKey, stringToSign).toHex();

    QString authHeader = QString("AWS4-HMAC-SHA256 Credential=%1/%2, "
                                 "SignedHeaders=%3, Signature=%4")
        .arg(accessKey, credentialScope, signedHeaders, signature);

    QMap<QString, QString> headers;
    headers["content-encoding"] = "amz-1.0";
    headers["host"]             = host;
    headers["x-amz-date"]       = dateTime;
    headers["x-amz-target"]     = target;
    headers["Authorization"]    = authHeader;
    return headers;
}

FetchResult AmazonHandler::parseResponse(const QByteArray& data, const std::string& asin) {
    FetchResult result;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull()) {
        result.errorMsg = "Invalid JSON response from Amazon API";
        Logger::error(result.errorMsg);
        return result;
    }

    // Response: ItemsResult.Items[0].Offers.Listings[0].Price.Amount
    QJsonObject itemsResult = doc.object()
        .value("ItemsResult").toObject();
    QJsonArray items = itemsResult.value("Items").toArray();
    if (items.isEmpty()) {
        result.errorMsg = "No items returned by Amazon API for ASIN: " + asin;
        Logger::warn(result.errorMsg);
        return result;
    }

    QJsonObject item     = items[0].toObject();
    QJsonObject offers   = item.value("Offers").toObject();
    QJsonArray  listings = offers.value("Listings").toArray();
    if (listings.isEmpty()) {
        result.errorMsg = "No listings for ASIN: " + asin;
        return result;
    }

    QJsonObject listing    = listings[0].toObject();
    QJsonObject priceObj   = listing.value("Price").toObject();
    result.price           = static_cast<float>(priceObj.value("Amount").toDouble());

    QJsonObject savingObj  = listing.value("SavingBasis").toObject();
    if (!savingObj.isEmpty()) {
        float original = static_cast<float>(savingObj.value("Amount").toDouble());
        if (original > 0)
            result.discount = (original - result.price) / original * 100.0f;
    }

    result.success = true;
    Logger::info("Amazon: ASIN=" + asin +
                 " price=$" + std::to_string(result.price));
    return result;
}

FetchResult AmazonHandler::fetchProduct(const std::string& url) {
    if (!validateUrl(url)) {
        return FetchResult{false, 0.0f, 0.0f, "Invalid URL for this handler"};
    }

    FetchResult result;

    std::string asin = extractAsin(url);
    if (asin.empty()) {
        result.errorMsg = "Could not extract ASIN from Amazon URL: " + url;
        Logger::warn(result.errorMsg);
        return result;
    }

    QString accessKey  = SettingsProvider::instance().amazonAccessKey();
    QString secretKey  = SettingsProvider::instance().amazonSecretKey();
    QString partnerTag = SettingsProvider::instance().amazonPartnerTag();

    if (accessKey.isEmpty() || secretKey.isEmpty() || partnerTag.isEmpty()) {
        result.errorMsg = "Amazon PA API credentials not configured. Set them in Settings.";
        Logger::warn(result.errorMsg);
        return result;
    }

    QByteArray payload = buildPayload(asin, partnerTag);
    QMap<QString, QString> headers = signRequest(payload, accessKey, secretKey);

    auto resp = m_http->postSync(QUrl(kAmazonApiUrl), payload, headers);
    if (!resp.ok) {
        result.errorMsg = resp.error.toStdString();
        Logger::error("Amazon fetch error: " + result.errorMsg);
        return result;
    }

    return parseResponse(resp.body, asin);
}
