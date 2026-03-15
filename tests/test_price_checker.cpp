#include "core/DataStructs.hpp"
#include "core/PriceChecker.hpp"
#include <cassert>
#include <iostream>

static void test_price_condition_met() {
    Product p;
    p.currentPrice = 29.99f;
    p.discount     = 0.0f;
    p.filters      = {{0, ConditionType::PRICE_LESS_EQUAL, 30.0f}};
    assert(PriceChecker::isMatch(p) && "price <= 30 should match when price=29.99");
}

static void test_price_condition_not_met() {
    Product p;
    p.currentPrice = 49.99f;
    p.discount     = 0.0f;
    p.filters      = {{0, ConditionType::PRICE_LESS_EQUAL, 30.0f}};
    assert(!PriceChecker::isMatch(p) && "price <= 30 should NOT match when price=49.99");
}

static void test_discount_condition_met() {
    Product p;
    p.currentPrice = 9.99f;
    p.discount     = 75.0f;
    p.filters      = {{0, ConditionType::DISCOUNT_GREATER_EQUAL, 50.0f}};
    assert(PriceChecker::isMatch(p) && "discount >= 50 should match when discount=75");
}

static void test_discount_condition_not_met() {
    Product p;
    p.currentPrice = 9.99f;
    p.discount     = 20.0f;
    p.filters      = {{0, ConditionType::DISCOUNT_GREATER_EQUAL, 50.0f}};
    assert(!PriceChecker::isMatch(p) && "discount >= 50 should NOT match when discount=20");
}

static void test_multiple_conditions_all_met() {
    Product p;
    p.currentPrice = 14.99f;
    p.discount     = 70.0f;
    p.filters      = {
        {0, ConditionType::PRICE_LESS_EQUAL,       20.0f},
        {0, ConditionType::DISCOUNT_GREATER_EQUAL, 50.0f}
    };
    assert(PriceChecker::isMatch(p) && "both conditions met");
}

static void test_multiple_conditions_one_fails() {
    Product p;
    p.currentPrice = 25.0f;
    p.discount     = 70.0f;
    p.filters      = {
        {0, ConditionType::PRICE_LESS_EQUAL,       20.0f}, // fails
        {0, ConditionType::DISCOUNT_GREATER_EQUAL, 50.0f}  // passes
    };
    assert(!PriceChecker::isMatch(p) && "should fail when any condition fails");
}

static void test_no_filters_always_match() {
    Product p;
    p.currentPrice = 999.0f;
    p.discount     = 0.0f;
    p.filters      = {};
    assert(PriceChecker::isMatch(p) && "no filters = always matches");
}

int main() {
    std::cout << "Running PriceChecker tests..." << std::endl;

    test_price_condition_met();
    test_price_condition_not_met();
    test_discount_condition_met();
    test_discount_condition_not_met();
    test_multiple_conditions_all_met();
    test_multiple_conditions_one_fails();
    test_no_filters_always_match();

    std::cout << "All PriceChecker tests passed." << std::endl;
    return 0;
}
