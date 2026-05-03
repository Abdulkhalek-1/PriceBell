// Tests AlertManager session dedup: alertTriggered fires only the FIRST time
// a product matches in this session, even though the DB save happens every
// time. Reset paths (resetNotificationFor / resetAllNotifications) re-arm.

#include <cassert>
#include <iostream>
#include <chrono>

#include <QCoreApplication>
#include <QSignalSpy>

#include "core/AlertManager.hpp"
#include "core/DataStructs.hpp"
#include "storage/Database.hpp"

static int s_failures = 0;
#define EXPECT(cond) do { \
    if (!(cond)) { std::cerr << "  FAIL: " << #cond << " (line " << __LINE__ << ")\n"; ++s_failures; } \
} while (0)

static Product makeProduct(int id, float threshold) {
    Product p;
    p.id   = id;
    p.name = "Test Product";
    p.url  = "https://example.com/p";
    p.source       = SourceType::GENERIC;
    p.checkInterval = std::chrono::seconds(60);
    p.isActive = true;
    PriceCondition c;
    c.type  = ConditionType::PRICE_LESS_EQUAL;
    c.value = threshold;
    p.filters.push_back(c);
    return p;
}

static FetchResult makeResult(float price) {
    FetchResult r;
    r.success  = true;
    r.price    = price;
    r.discount = 0.0f;
    return r;
}

void test_first_match_fires_signal() {
    AlertManager mgr;
    QSignalSpy spy(&mgr, &AlertManager::alertTriggered);

    auto p = makeProduct(1, 50.0f);
    mgr.onPriceUpdated(p, makeResult(30.0f));   // price <= 50 → match

    EXPECT(spy.count() == 1);
    if (spy.count() == 1) {
        EXPECT(spy.at(0).at(1).toBool() == true); // firstThisSession
    }
    std::cout << "  PASS: test_first_match_fires_signal\n";
}

void test_repeat_match_dedupes() {
    AlertManager mgr;
    QSignalSpy spy(&mgr, &AlertManager::alertTriggered);

    auto p = makeProduct(2, 50.0f);
    mgr.onPriceUpdated(p, makeResult(30.0f));   // first match — fires
    mgr.onPriceUpdated(p, makeResult(20.0f));   // still matches — should not re-fire as "first"

    EXPECT(spy.count() == 2); // signal still fires (so the row updates) but with firstThisSession=false
    if (spy.count() == 2) {
        EXPECT(spy.at(0).at(1).toBool() == true);
        EXPECT(spy.at(1).at(1).toBool() == false);
    }
    std::cout << "  PASS: test_repeat_match_dedupes\n";
}

void test_non_match_resets_dedup() {
    AlertManager mgr;
    QSignalSpy spy(&mgr, &AlertManager::alertTriggered);

    auto p = makeProduct(3, 50.0f);
    mgr.onPriceUpdated(p, makeResult(30.0f));   // match → fires firstThisSession=true
    mgr.onPriceUpdated(p, makeResult(60.0f));   // no match → clears dedup
    mgr.onPriceUpdated(p, makeResult(40.0f));   // matches again → should fire firstThisSession=true

    EXPECT(spy.count() == 2);
    if (spy.count() == 2) {
        EXPECT(spy.at(0).at(1).toBool() == true);
        EXPECT(spy.at(1).at(1).toBool() == true);
    }
    std::cout << "  PASS: test_non_match_resets_dedup\n";
}

void test_explicit_reset() {
    AlertManager mgr;
    QSignalSpy spy(&mgr, &AlertManager::alertTriggered);

    auto p = makeProduct(4, 50.0f);
    mgr.onPriceUpdated(p, makeResult(30.0f));
    mgr.resetNotificationFor(4);
    mgr.onPriceUpdated(p, makeResult(30.0f));

    EXPECT(spy.count() == 2);
    if (spy.count() == 2) {
        EXPECT(spy.at(0).at(1).toBool() == true);
        EXPECT(spy.at(1).at(1).toBool() == true);
    }
    std::cout << "  PASS: test_explicit_reset\n";
}

void test_unsuccessful_result_ignored() {
    AlertManager mgr;
    QSignalSpy spy(&mgr, &AlertManager::alertTriggered);
    auto p = makeProduct(5, 50.0f);
    FetchResult r;
    r.success = false;
    r.errorMsg = "boom";
    mgr.onPriceUpdated(p, r);
    EXPECT(spy.count() == 0);
    std::cout << "  PASS: test_unsuccessful_result_ignored\n";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("PriceBellTest");
    QCoreApplication::setApplicationName("test_alert_dedup");

    // AlertManager persists every match to the DB; open an in-memory DB so the
    // call doesn't hit disk and the test stays hermetic.
    Database::open(":memory:");

    std::cout << "Running test_alert_dedup...\n";
    test_first_match_fires_signal();
    test_repeat_match_dedupes();
    test_non_match_resets_dedup();
    test_explicit_reset();
    test_unsuccessful_result_ignored();

    Database::close();

    if (s_failures > 0) {
        std::cerr << "FAILED: " << s_failures << " assertion(s)\n";
        return 1;
    }
    std::cout << "All test_alert_dedup tests passed\n";
    return 0;
}
