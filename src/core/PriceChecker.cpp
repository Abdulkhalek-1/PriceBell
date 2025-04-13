#include "core/DataStructs.hpp"
#include <iostream>

bool isMatch(const Product& product) {
    for (const auto& filter : product.filters) {
        if (filter.type == PriceCondition::Type::PRICE_GREATER_EQUAL &&
            product.currentPrice < filter.value) return false;
        if (filter.type == PriceCondition::Type::DISCOUNT_GREATER_EQUAL &&
            product.discount < filter.value) return false;
    }
    return true;
}
