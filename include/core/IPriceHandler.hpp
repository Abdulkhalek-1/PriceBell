#pragma once
#include "DataStructs.hpp"
#include <optional>

class IPriceHandler {
public:
    virtual std::optional<Product> fetchProduct(const std::string& url) = 0;
    virtual ~IPriceHandler() = default;
};
