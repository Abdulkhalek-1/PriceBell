// NOTE: This test needs to be registered in CMakeLists.txt (Track D)
#include "utils/SettingsProvider.hpp"
#include <QCoreApplication>
#include <QSettings>
#include <cassert>
#include <iostream>

// ── Test: default values ────────────────────────────────────────────────────

static void test_default_values() {
    // Clear any previous state
    QSettings settings("PriceBell", "PriceBell");
    settings.clear();

    auto& sp = SettingsProvider::instance();

    assert(sp.defaultInterval() == 3600 && "default interval should be 3600");
    assert(sp.language() == "en" && "default language should be 'en'");
    assert(sp.notificationSoundEnabled() == true && "notifications should be enabled by default");
    assert(sp.checkUpdatesOnStartup() == true && "check-updates should be enabled by default");

    std::cout << "  PASS: test_default_values" << std::endl;
}

// ── Test: set and get ───────────────────────────────────────────────────────

static void test_set_and_get() {
    auto& sp = SettingsProvider::instance();

    sp.setDefaultInterval(1800);
    assert(sp.defaultInterval() == 1800 && "interval should be 1800 after set");

    sp.setLanguage("fr");
    assert(sp.language() == "fr" && "language should be 'fr' after set");

    sp.setNotificationSoundEnabled(false);
    assert(sp.notificationSoundEnabled() == false && "sound should be false after set");

    sp.setAmazonPartnerTag("my-tag-20");
    assert(sp.amazonPartnerTag() == "my-tag-20" && "partner tag should round-trip");

    std::cout << "  PASS: test_set_and_get" << std::endl;

    // Cleanup
    QSettings settings("PriceBell", "PriceBell");
    settings.clear();
}

// ── Test: credential encoding ───────────────────────────────────────────────

static void test_credential_encoding() {
    QSettings settings("PriceBell", "PriceBell");
    settings.clear();

    auto& sp = SettingsProvider::instance();
    sp.setUdemyClientId("my_secret_id");

    // Verify the stored value is NOT plaintext
    QString raw = settings.value("udemy/client_id").toString();
    assert(raw != "my_secret_id" && "credential should not be stored as plaintext");

    // Verify we can read it back correctly
    assert(sp.udemyClientId() == "my_secret_id" && "decoded credential should match original");

    // Verify it's base64
    QByteArray decoded = QByteArray::fromBase64(raw.toUtf8());
    assert(QString::fromUtf8(decoded) == "my_secret_id" && "raw value should be valid base64 of original");

    std::cout << "  PASS: test_credential_encoding" << std::endl;

    settings.clear();
}

// ── Main ────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName("PriceBell");
    app.setOrganizationName("PriceBell");

    std::cout << "Running SettingsProvider tests..." << std::endl;

    test_default_values();
    test_set_and_get();
    test_credential_encoding();

    std::cout << "All SettingsProvider tests passed." << std::endl;
    return 0;
}
