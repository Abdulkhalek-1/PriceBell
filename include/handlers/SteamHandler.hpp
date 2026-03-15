#pragma once

#include "core/IPriceHandler.hpp"
#include "utils/HttpClient.hpp"

// Fetches price data from the Steam Store API.
// Endpoint: https://store.steampowered.com/api/appdetails?appids=<appid>&cc=us&filters=price_overview
class SteamHandler : public IPriceHandler {
public:
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override { return "steam"; }
    std::string displayName() const override { return "Steam"; }

private:
    // Extracts the Steam app ID from a store URL.
    // e.g. https://store.steampowered.com/app/730/... → "730"
    static std::string extractAppId(const std::string& url);
};
