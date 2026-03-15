#pragma once

#include "core/DataStructs.hpp"
#include <vector>

class AlertRepository {
public:
    // Saves a new alert event. Sets event.id on success.
    static bool save(AlertEvent& event);

    // Returns all alert events ordered by triggered_at DESC.
    static std::vector<AlertEvent> findAll();

    // Returns all alert events for a specific product.
    static std::vector<AlertEvent> findByProduct(int productId);

    // Marks an alert as dismissed.
    static bool dismiss(int alertId);

    // Removes all alerts for a given product (used when product is deleted).
    static bool removeByProduct(int productId);
};
