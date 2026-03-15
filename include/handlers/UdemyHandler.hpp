#pragma once

#include "core/IPriceHandler.hpp"
#include "utils/HttpClient.hpp"

// Fetches price data from the Udemy API.
// Requires Udemy client credentials configured in SettingsDialog.
class UdemyHandler : public IPriceHandler {
public:
    explicit UdemyHandler(HttpClient* http = nullptr);

    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override { return "udemy"; }
    std::string displayName() const override { return "Udemy"; }
    void setHttpClient(HttpClient* http) override { m_http = http; }

private:
    HttpClient* m_http;

    static std::string extractCourseId(const std::string& url);

    // Validates that the URL is a legitimate Udemy course URL.
    bool validateUrl(const std::string& url) const;
};
