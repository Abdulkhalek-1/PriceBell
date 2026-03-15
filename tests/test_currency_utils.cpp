#include <cassert>
#include <iostream>
#include "../include/utils/CurrencyUtils.hpp"

void test_format_usd() {
    assert(CurrencyUtils::formatPrice(49.99f, "USD") == "$49.99");
}
void test_format_eur() {
    QString result = CurrencyUtils::formatPrice(49.99f, "EUR");
    assert(result.contains("\u20AC") || result.contains("EUR"));
}
void test_format_gbp() {
    assert(CurrencyUtils::formatPrice(29.99f, "GBP").startsWith("\u00A3"));
}
void test_symbol_usd() {
    assert(CurrencyUtils::currencySymbol("USD") == "$");
}
void test_unknown_currency() {
    assert(CurrencyUtils::currencySymbol("XYZ") == "XYZ");
}

int main() {
    std::cout << "Running CurrencyUtils tests..." << std::endl;

    test_format_usd();
    test_format_eur();
    test_format_gbp();
    test_symbol_usd();
    test_unknown_currency();

    std::cout << "All CurrencyUtils tests passed." << std::endl;
    return 0;
}
