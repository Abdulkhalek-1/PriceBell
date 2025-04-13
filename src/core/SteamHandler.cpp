#include "core/SteamHandler.hpp"
#include <iostream>

std::optional<Product> SteamHandler::fetchProduct(const std::string& url) {
    // Mocked for now
    std::cout << "Fetching from Steam: " << url << std::endl;
    return Product{
        1, "Half-Life 3", "steam", 59.99f, 10.0f,
        { {PriceCondition::Type::PRICE_GREATER_EQUAL, 50.0f} },
        std::chrono::seconds(3600), std::chrono::system_clock::now()
    };
}
