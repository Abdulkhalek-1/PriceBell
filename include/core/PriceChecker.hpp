#pragma once

#include "core/DataStructs.hpp"

// Evaluates whether a product's current price/discount satisfies all its filter conditions.
class PriceChecker {
public:
    // Returns true if every PriceCondition in product.filters is satisfied.
    static bool isMatch(const Product& product);
};
