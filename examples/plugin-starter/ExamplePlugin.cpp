#include "ExamplePlugin.hpp"
#include <QJsonArray>

FetchResult ExamplePlugin::fetchProduct(const std::string& /*url*/) {
    // Replace this with real HTTP fetching logic.
    // HttpClient is available — see src/utils/HttpClient.cpp for usage.
    FetchResult result;
    result.success  = true;
    result.price    = 9.99f;
    result.discount = 0.0f;
    return result;
}

std::string ExamplePlugin::handlerId() const {
    // Must be unique across all loaded plugins.
    return "example";
}

std::string ExamplePlugin::displayName() const {
    return "Example Plugin";
}

QJsonObject ExamplePlugin::metadata() const {
    // Must match the fields in example-plugin.json
    QJsonArray patterns;
    patterns.append(QString("https://example.com/*"));
    QJsonObject obj;
    obj["id"]          = "example";
    obj["name"]        = "Example Plugin";
    obj["version"]     = "1.0";
    obj["urlPatterns"] = patterns;
    return obj;
}
