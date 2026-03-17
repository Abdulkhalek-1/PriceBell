#include "core/AlertManager.hpp"
#include "core/PriceChecker.hpp"
#include "storage/AlertRepository.hpp"
#include "utils/Logger.hpp"

#include <chrono>

AlertManager::AlertManager(QObject* parent)
    : QObject(parent)
{}

void AlertManager::onPriceUpdated(Product product, FetchResult result) {
    if (!result.success) return;

    // Apply the fetched price/discount to the product
    product.currentPrice = result.price;
    product.discount     = result.discount;

    if (!PriceChecker::isMatch(product)) return;

    // All conditions met — create and persist the alert event
    AlertEvent event;
    event.productId         = product.id;
    event.productName       = product.name;
    event.priceAtTrigger    = result.price;
    event.discountAtTrigger = result.discount;
    event.triggeredAt       = std::chrono::system_clock::now();
    event.status            = AlertStatus::TRIGGERED;

    if (!AlertRepository::save(event)) {
        Logger::warn("Failed to save alert event for product: " + event.productName);
    }

    Logger::info("Alert triggered: " + product.name +
                 " @ $" + std::to_string(result.price));

    emit alertTriggered(event);
}
