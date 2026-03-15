#include "core/PluginManager.hpp"
#include <QJsonObject>
#include <QJsonArray>
#include <cassert>
#include <iostream>

static QJsonObject makeValidMeta() {
    QJsonObject meta;
    meta["id"] = "my-custom-plugin";
    meta["name"] = "My Custom Plugin";
    meta["version"] = "1.0";
    QJsonArray patterns;
    patterns.append("https://example.com/*");
    meta["urlPatterns"] = patterns;
    return meta;
}

static void test_metadata_valid() {
    QJsonObject meta = makeValidMeta();
    assert(PluginManager::validatePluginMetadata(meta) && "Valid metadata should pass");
}

static void test_metadata_missing_id() {
    QJsonObject meta = makeValidMeta();
    meta.remove("id");
    assert(!PluginManager::validatePluginMetadata(meta) && "Missing id should be rejected");
}

static void test_metadata_missing_url_patterns() {
    QJsonObject meta = makeValidMeta();
    meta.remove("urlPatterns");
    assert(!PluginManager::validatePluginMetadata(meta) && "Missing urlPatterns should be rejected");
}

static void test_metadata_builtin_id_rejected() {
    for (const char* builtinId : {"steam", "udemy", "amazon", "generic"}) {
        QJsonObject meta = makeValidMeta();
        meta["id"] = builtinId;
        assert(!PluginManager::validatePluginMetadata(meta)
               && "Built-in id should be rejected");
    }
}

static void test_metadata_broad_pattern_rejected() {
    for (const char* broad : {"*", "http://*", "https://*"}) {
        QJsonObject meta = makeValidMeta();
        QJsonArray patterns;
        patterns.append(broad);
        meta["urlPatterns"] = patterns;
        assert(!PluginManager::validatePluginMetadata(meta)
               && "Broad pattern should be rejected");
    }
}

static void test_metadata_empty_patterns_rejected() {
    QJsonObject meta = makeValidMeta();
    meta["urlPatterns"] = QJsonArray();
    assert(!PluginManager::validatePluginMetadata(meta)
           && "Empty urlPatterns array should be rejected");
}

int main() {
    std::cout << "Running plugin validation tests..." << std::endl;

    test_metadata_valid();
    test_metadata_missing_id();
    test_metadata_missing_url_patterns();
    test_metadata_builtin_id_rejected();
    test_metadata_broad_pattern_rejected();
    test_metadata_empty_patterns_rejected();

    std::cout << "All plugin validation tests passed." << std::endl;
    return 0;
}
