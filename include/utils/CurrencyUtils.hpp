#pragma once
#include <string>
#include <QString>

namespace CurrencyUtils {

inline QString currencySymbol(const std::string& code) {
    if (code == "USD") return "$";
    if (code == "EUR") return "\u20AC";
    if (code == "GBP") return "\u00A3";
    if (code == "CAD") return "CA$";
    if (code == "AUD") return "A$";
    if (code == "JPY") return "\u00A5";
    if (code == "TRY") return "\u20BA";
    if (code == "BRL") return "R$";
    if (code == "INR") return "\u20B9";
    if (code == "SAR") return "SAR";
    if (code == "AED") return "AED";
    if (code == "EGP") return "EGP";
    return QString::fromStdString(code); // fallback to code itself
}

inline QString formatPrice(float price, const std::string& currency) {
    QString symbol = currencySymbol(currency);
    // Currencies where symbol comes before amount
    if (currency == "USD" || currency == "GBP" || currency == "CAD" ||
        currency == "AUD" || currency == "JPY" || currency == "INR" ||
        currency == "BRL" || currency == "EGP") {
        return QString("%1%2").arg(symbol).arg(price, 0, 'f', 2);
    }
    // Currencies where symbol/code comes after amount
    return QString("%1 %2").arg(price, 0, 'f', 2).arg(symbol);
}

} // namespace CurrencyUtils
