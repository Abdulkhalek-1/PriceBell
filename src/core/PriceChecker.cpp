#include "core/PriceChecker.hpp"

bool PriceChecker::isMatch(const Product& product) {
    for (const auto& filter : product.filters) {
        switch (filter.type) {
            case ConditionType::PRICE_LESS_EQUAL:
                if (product.currentPrice > filter.value) return false;
                break;
            case ConditionType::DISCOUNT_GREATER_EQUAL:
                if (product.discount < filter.value) return false;
                break;
        }
    }
    return true;
}
