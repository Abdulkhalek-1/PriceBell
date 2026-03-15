#include "storage/Database.hpp"
#include "storage/ProductRepository.hpp"
#include "storage/AlertRepository.hpp"
#include <cassert>
#include <iostream>

static void setup() {
    // Use an in-memory SQLite database for tests
    bool ok = Database::open(":memory:");
    assert(ok && "Database should open in-memory");
}

static void test_save_and_find_product() {
    Product p;
    p.name          = "Test Game";
    p.url           = "https://store.steampowered.com/app/123";
    p.source        = SourceType::STEAM;
    p.currentPrice  = 39.99f;
    p.discount      = 20.0f;
    p.isActive      = true;
    p.checkInterval = std::chrono::seconds(1800);
    p.filters       = {{0, ConditionType::PRICE_LESS_EQUAL, 20.0f}};

    bool saved = ProductRepository::save(p);
    assert(saved && "Product should save successfully");
    assert(p.id > 0 && "Product id should be set after save");

    auto found = ProductRepository::findById(p.id);
    assert(found.has_value() && "Should find saved product by id");
    assert(found->name == "Test Game" && "Name should match");
    assert(found->filters.size() == 1 && "Conditions should be loaded");
}

static void test_find_all() {
    auto all = ProductRepository::findAll();
    assert(!all.empty() && "findAll should return at least one product");
}

static void test_update_product() {
    auto all = ProductRepository::findAll();
    assert(!all.empty());

    Product p  = all[0];
    p.currentPrice = 9.99f;
    bool ok = ProductRepository::update(p);
    assert(ok && "Update should succeed");

    auto found = ProductRepository::findById(p.id);
    assert(found.has_value());
    assert(std::abs(found->currentPrice - 9.99f) < 0.01f && "Updated price should match");
}

static void test_save_and_find_alert() {
    auto all = ProductRepository::findAll();
    assert(!all.empty());

    AlertEvent e;
    e.productId         = all[0].id;
    e.productName       = all[0].name;
    e.priceAtTrigger    = 9.99f;
    e.discountAtTrigger = 50.0f;
    e.triggeredAt       = std::chrono::system_clock::now();
    e.status            = AlertStatus::TRIGGERED;

    bool saved = AlertRepository::save(e);
    assert(saved && "Alert should save successfully");
    assert(e.id > 0 && "Alert id should be set");

    auto alerts = AlertRepository::findAll();
    assert(!alerts.empty() && "findAll alerts should return results");

    auto byProduct = AlertRepository::findByProduct(all[0].id);
    assert(!byProduct.empty() && "findByProduct should find the alert");
}

static void test_dismiss_alert() {
    auto alerts = AlertRepository::findAll();
    assert(!alerts.empty());

    bool dismissed = AlertRepository::dismiss(alerts[0].id);
    assert(dismissed && "Dismiss should succeed");

    auto updated = AlertRepository::findAll();
    assert(updated[0].status == AlertStatus::DISMISSED && "Status should be dismissed");
}

static void test_remove_product_cascades() {
    auto all = ProductRepository::findAll();
    assert(!all.empty());
    int id = all[0].id;

    bool removed = ProductRepository::remove(id);
    assert(removed && "Remove should succeed");

    auto found = ProductRepository::findById(id);
    assert(!found.has_value() && "Product should not exist after removal");

    // Cascade: alert history for this product should also be gone
    auto alerts = AlertRepository::findByProduct(id);
    assert(alerts.empty() && "Alerts should cascade-delete with product");
}

int main() {
    std::cout << "Running Repository tests..." << std::endl;

    setup();
    test_save_and_find_product();
    test_find_all();
    test_update_product();
    test_save_and_find_alert();
    test_dismiss_alert();
    test_remove_product_cascades();

    Database::close();
    std::cout << "All Repository tests passed." << std::endl;
    return 0;
}
