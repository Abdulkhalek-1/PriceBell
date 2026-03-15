#pragma once

#include "core/IPriceHandler.hpp"

// Fetches price data from the Amazon Product Advertising API 5.0.
// Requires AWS Access Key + Secret configured in SettingsDialog.
class AmazonHandler : public IPriceHandler {
public:
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override { return "amazon"; }
    std::string displayName() const override { return "Amazon"; }

private:
    static std::string extractAsin(const std::string& url);
};
