#pragma once

#include "DataStructs.hpp"
#include <string>

// Strategy interface for all price-fetching implementations.
// Every built-in handler and developer plugin must implement this.
class IPriceHandler {
public:
    virtual ~IPriceHandler() = default;

    // Fetch the current price and discount for the given product URL.
    virtual FetchResult fetchProduct(const std::string& url) = 0;

    // Unique identifier for this handler (e.g. "steam", "udemy", "my-plugin").
    virtual std::string handlerId() const = 0;

    // Human-readable display name shown in the UI source selector.
    virtual std::string displayName() const = 0;
};
