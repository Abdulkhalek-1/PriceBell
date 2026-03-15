#pragma once

#include "core/IPriceHandler.hpp"
#include "core/DataStructs.hpp"
#include "utils/HttpClient.hpp"

// Fetches price from any URL using a user-configured JSON path or CSS selector.
// Used for non-technical custom sources stored in the sources table.
class GenericWebHandler : public IPriceHandler {
public:
    explicit GenericWebHandler(const SourceConfig& config, HttpClient* http = nullptr);

    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override { return m_config.id; }
    std::string displayName() const override { return m_config.name; }
    void setHttpClient(HttpClient* http) override { m_http = http; }

private:
    SourceConfig m_config;
    HttpClient* m_http;

    // Extracts a value from a JSON string using a simple dot-notation path.
    // e.g. "price_overview.final" -> extracts the nested value.
    static float extractJsonPath(const std::string& json, const std::string& path);

    // Validates that the URL uses http or https scheme.
    bool validateUrl(const std::string& url) const;
};
