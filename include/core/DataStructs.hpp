#pragma once
#include <string>
#include <vector>
#include <chrono>

struct PriceCondition {
    enum class Type { PRICE_GREATER_EQUAL, DISCOUNT_GREATER_EQUAL };
    Type type;
    float value;
};

struct Product {
    int id;
    std::string name;
    std::string source; // e.g., "steam", "udemy"
    float currentPrice;
    float discount;
    std::vector<PriceCondition> filters;
    std::chrono::seconds checkInterval;
    std::chrono::system_clock::time_point lastChecked;
};
