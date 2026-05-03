// Tests for PluginManager::findHandlerForUrl — the auto-detect entry point
// used by ProductDialog when the user pastes a URL.

#include <cassert>
#include <iostream>

#include <QCoreApplication>
#include <QSettings>

#include "core/PluginManager.hpp"
#include "handlers/SteamHandler.hpp"
#include "handlers/UdemyHandler.hpp"
#include "handlers/AmazonHandler.hpp"

static int s_failures = 0;
#define EXPECT(cond) do { \
    if (!(cond)) { std::cerr << "  FAIL: " << #cond << " (" << __LINE__ << ")\n"; ++s_failures; } \
} while (0)

void test_steam_url_detected() {
    PluginManager pm;
    pm.registerBuiltins();
    auto* h = pm.findHandlerForUrl("https://store.steampowered.com/app/730/Counter-Strike_2/");
    EXPECT(h != nullptr);
    EXPECT(h && h->handlerId() == "steam");
    std::cout << "  PASS: test_steam_url_detected\n";
}

void test_udemy_url_detected() {
    PluginManager pm;
    pm.registerBuiltins();
    auto* h = pm.findHandlerForUrl("https://www.udemy.com/course/foo-bar/");
    EXPECT(h != nullptr);
    EXPECT(h && h->handlerId() == "udemy");
    std::cout << "  PASS: test_udemy_url_detected\n";
}

void test_amazon_url_detected() {
    PluginManager pm;
    pm.registerBuiltins();
    auto* h = pm.findHandlerForUrl("https://www.amazon.com/dp/B08L5M9BTJ");
    EXPECT(h != nullptr);
    EXPECT(h && h->handlerId() == "amazon");
    std::cout << "  PASS: test_amazon_url_detected\n";
}

void test_unrelated_url_returns_null() {
    PluginManager pm;
    pm.registerBuiltins();
    auto* h = pm.findHandlerForUrl("https://example.com/products/widget");
    EXPECT(h == nullptr);
    std::cout << "  PASS: test_unrelated_url_returns_null\n";
}

void test_empty_url_returns_null() {
    PluginManager pm;
    pm.registerBuiltins();
    EXPECT(pm.findHandlerForUrl("") == nullptr);
    std::cout << "  PASS: test_empty_url_returns_null\n";
}

void test_clone_independent() {
    SteamHandler h(nullptr);
    auto cloned = h.clone();
    EXPECT(cloned != nullptr);
    EXPECT(cloned.get() != &h);
    EXPECT(cloned->handlerId() == "steam");
    std::cout << "  PASS: test_clone_independent\n";
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    QCoreApplication::setOrganizationName("PriceBellTest");
    QCoreApplication::setApplicationName("test_url_routing");

    std::cout << "Running test_url_routing...\n";
    test_steam_url_detected();
    test_udemy_url_detected();
    test_amazon_url_detected();
    test_unrelated_url_returns_null();
    test_empty_url_returns_null();
    test_clone_independent();

    if (s_failures > 0) {
        std::cerr << "FAILED: " << s_failures << " assertion(s)\n";
        return 1;
    }
    std::cout << "All test_url_routing tests passed\n";
    return 0;
}
