#include "handlers/AmazonHandler.hpp"
#include "utils/Logger.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <QEventLoop>
#include <QSettings>
#include <QDateTime>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>
#include <regex>

// Amazon Product Advertising API 5.0 (PA API)
// Docs: https://webservices.amazon.com/paapi5/documentation/
static const char* kAmazonApiUrl  = "https://webservices.amazon.com/paapi5/getitems";
static const char* kAmazonRegion  = "us-east-1";
static const char* kAmazonService = "ProductAdvertisingAPI";

std::string AmazonHandler::extractAsin(const std::string& url) {
    // Match /dp/XXXXXXXXXX or /gp/product/XXXXXXXXXX
    std::regex re(R"(/(?:dp|gp/product)/([A-Z0-9]{10}))");
    std::smatch m;
    if (std::regex_search(url, m, re) && m.size() > 1)
        return m[1].str();
    return {};
}

FetchResult AmazonHandler::fetchProduct(const std::string& url) {
    FetchResult result;

    std::string asin = extractAsin(url);
    if (asin.empty()) {
        result.errorMsg = "Could not extract ASIN from Amazon URL: " + url;
        Logger::warn(result.errorMsg);
        return result;
    }

    QSettings settings("PriceBell", "PriceBell");
    QString accessKey   = settings.value("amazon/access_key").toString();
    QString secretKey   = settings.value("amazon/secret_key").toString();
    QString partnerTag  = settings.value("amazon/partner_tag").toString();

    if (accessKey.isEmpty() || secretKey.isEmpty() || partnerTag.isEmpty()) {
        result.errorMsg = "Amazon PA API credentials not configured. Set them in Settings.";
        Logger::warn(result.errorMsg);
        return result;
    }

    // Build the JSON payload for GetItems
    QString payload = QString(R"({
        "ItemIds": ["%1"],
        "Resources": ["Offers.Listings.Price", "Offers.Listings.SavingBasis"],
        "PartnerTag": "%2",
        "PartnerType": "Associates",
        "Marketplace": "www.amazon.com"
    })").arg(QString::fromStdString(asin)).arg(partnerTag);

    // AWS Signature V4
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
        payload.toUtf8(), QCryptographicHash::Sha256).toHex();

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

    QNetworkAccessManager mgr;
    QUrl _url(kAmazonApiUrl); QNetworkRequest request{_url};
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    request.setRawHeader("content-encoding",  "amz-1.0");
    request.setRawHeader("host",              host.toUtf8());
    request.setRawHeader("x-amz-date",        dateTime.toUtf8());
    request.setRawHeader("x-amz-target",      target.toUtf8());
    request.setRawHeader("Authorization",     authHeader.toUtf8());
    request.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell/2.0");

    QEventLoop loop;
    QNetworkReply* reply = mgr.post(request, payload.toUtf8());
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() != QNetworkReply::NoError) {
        result.errorMsg = reply->errorString().toStdString();
        Logger::error("Amazon fetch error: " + result.errorMsg);
        reply->deleteLater();
        return result;
    }

    QJsonDocument doc  = QJsonDocument::fromJson(reply->readAll());
    reply->deleteLater();

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
