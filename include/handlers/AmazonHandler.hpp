#pragma once

#include "core/IPriceHandler.hpp"
#include "utils/HttpClient.hpp"

#include <QByteArray>
#include <QNetworkRequest>

// Fetches price data from the Amazon Product Advertising API 5.0.
// Requires AWS Access Key + Secret configured in SettingsDialog.
class AmazonHandler : public IPriceHandler {
public:
    explicit AmazonHandler(HttpClient* http = nullptr);

    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override { return "amazon"; }
    std::string displayName() const override { return "Amazon"; }
    void setHttpClient(HttpClient* http) override { m_http = http; }

private:
    HttpClient* m_http;

    static std::string extractAsin(const std::string& url);

    // Validates that the URL is a legitimate Amazon URL.
    bool validateUrl(const std::string& url) const;

    // Builds the PA API JSON request body for GetItems.
    QByteArray buildPayload(const std::string& asin, const QString& partnerTag);

    // Creates a signed QNetworkRequest with AWS SigV4 headers.
    // Returns the headers map for use with HttpClient::postSync().
    QMap<QString, QString> signRequest(const QByteArray& payload,
                                       const QString& accessKey,
                                       const QString& secretKey);

    // Extracts price/discount from the PA API JSON response.
    FetchResult parseResponse(const QByteArray& data, const std::string& asin);
};
