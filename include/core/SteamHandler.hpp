#pragma once
#include "IPriceHandler.hpp"

class SteamHandler : public IPriceHandler {
public:
    std::optional<Product> fetchProduct(const std::string& url) override;
};
