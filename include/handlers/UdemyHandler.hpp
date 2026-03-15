#pragma once

#include "core/IPriceHandler.hpp"

// Fetches price data from the Udemy API.
// Requires Udemy client credentials configured in SettingsDialog.
class UdemyHandler : public IPriceHandler {
public:
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override { return "udemy"; }
    std::string displayName() const override { return "Udemy"; }

private:
    static std::string extractCourseId(const std::string& url);
};
