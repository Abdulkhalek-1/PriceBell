# Plugin Development Guide

## Overview

PriceBell supports two types of plugins for adding custom price sources:

1. **Native plugins** -- compiled `.so`/`.dll` Qt plugins for developers who need full control over how prices are fetched and parsed.
2. **JSON config sources** -- simple configuration-based sources for non-technical users who want to add a price API without writing code.

There are two plugin interfaces available:

- **IPlugin** -- basic interface for price fetching only.
- **IPlugin2** -- extended interface with optional UI integration (settings widgets, icons, stylesheets, data models).

---

## Native Plugin Development

### Basic Plugin (IPlugin)

To create a basic native plugin, subclass both `QObject` and `IPlugin`:

```cpp
#include <QObject>
#include <QtPlugin>
#include "core/IPlugin.hpp"

class MyStoreHandler : public QObject, public IPlugin {
    Q_OBJECT
    Q_INTERFACES(IPlugin)
    Q_PLUGIN_METADATA(IID "com.pricebell.IPlugin/1.0" FILE "meta.json")

public:
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId() const override { return "mystore"; }
    std::string displayName() const override { return "My Store"; }
    void setHttpClient(HttpClient* http) override { m_http = http; }
    QJsonObject metadata() const override;

private:
    HttpClient* m_http = nullptr;
};
```

### Extended Plugin (IPlugin2)

For plugins that need richer UI integration, implement `IPlugin2` instead:

```cpp
#include <QObject>
#include <QtPlugin>
#include "core/IPlugin2.hpp"

class MyStoreHandler : public QObject, public IPlugin2 {
    Q_OBJECT
    Q_INTERFACES(IPlugin2)
    Q_PLUGIN_METADATA(IID "com.pricebell.IPlugin2/1.0" FILE "meta.json")

public:
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId() const override { return "mystore"; }
    std::string displayName() const override { return "My Store"; }
    void setHttpClient(HttpClient* http) override { m_http = http; }
    QJsonObject metadata() const override;

    // IPlugin2 optional overrides
    QWidget* settingsWidget() override;   // Embedded in Settings > Plugins tab
    QIcon icon() const override;          // Shown in source selector
    QString styleSheet() const override;  // QSS for plugin UI elements
    QJsonObject dataModel() const override; // Custom data field descriptors

private:
    HttpClient* m_http = nullptr;
};
```

### IPlugin2 Methods

| Method | Return Type | Default | Description |
| --- | --- | --- | --- |
| `settingsWidget()` | `QWidget*` | `nullptr` | Returns a widget embedded in the Settings dialog Plugins tab. PriceBell takes ownership. |
| `icon()` | `QIcon` | empty | Icon displayed in the source selector and handler list. |
| `styleSheet()` | `QString` | empty | QSS injected for plugin UI elements. |
| `dataModel()` | `QJsonObject` | empty | Describes custom data fields for generic UI rendering. |

### HttpClient Dependency Injection

All handlers (built-in and plugin) receive an `HttpClient*` via `setHttpClient()` before `fetchProduct()` is called. The `PluginManager` creates the `HttpClient` lazily on the calling thread (the poller thread) to satisfy Qt's thread-affinity requirements for `QNetworkAccessManager`.

Your plugin should store and use this client for all HTTP requests rather than creating its own `HttpClient` instance.

### Plugin Metadata (meta.json)

Every native plugin must include a `meta.json` file alongside its source. This file is embedded at compile time via `Q_PLUGIN_METADATA`.

```json
{
    "id": "mystore",
    "name": "My Store",
    "version": "1.0",
    "urlPatterns": ["https://mystore.com/product/*"]
}
```

### IPlugin Interface

`IPlugin` extends `IPriceHandler` and requires the following methods:

| Method | Return Type | Description |
|---|---|---|
| `fetchProduct(url)` | `FetchResult` | Fetch price and discount information from the given URL. |
| `handlerId()` | `std::string` | Unique string identifier. Must not conflict with built-in IDs: `steam`, `udemy`, `amazon`, `generic`. |
| `displayName()` | `std::string` | Human-readable name shown in the UI. |
| `setHttpClient(http)` | `void` | Receive the shared `HttpClient` instance for network requests. |
| `metadata()` | `QJsonObject` | Returns plugin metadata including `id`, `name`, `version`, and `urlPatterns`. |

### FetchResult Struct

All handlers (both built-in and plugin) return a `FetchResult`:

```cpp
struct FetchResult {
    bool success = false;
    float price = 0.0f;
    float discount = 0.0f;
    std::string errorMsg;
};
```

Set `success` to `true` and populate `price` and `discount` on a successful fetch. On failure, set `success` to `false` and provide a descriptive `errorMsg`.

### Building a Plugin

Compile the plugin as a shared library, linking against `Qt5::Core` and the PriceBell headers:

```cmake
add_library(mystore_plugin SHARED MyStoreHandler.cpp)
target_link_libraries(mystore_plugin Qt5::Core)
target_include_directories(mystore_plugin PRIVATE /path/to/pricebell/include)
```

### Installation

Drop the compiled shared library (`.so` on Linux, `.dll` on Windows, `.dylib` on macOS) into the `plugins/` directory. The plugin directory path is configurable in Settings.

### Plugin Discovery

`PluginManager::loadPlugins()` scans the configured plugin directory for shared libraries. For each file it finds, it:

1. Loads the library using `QPluginLoader`.
2. Validates Qt ABI compatibility.
3. Reads and validates the plugin metadata.
4. Registers the handler for use by `PriceChecker`.

### Settings Integration (IPlugin2)

If your plugin implements `IPlugin2::settingsWidget()`, the returned widget is embedded directly in the Settings dialog under the **Plugins** tab. Each plugin handler appears as a styled row with its icon, name, ID, and a "Plugin" badge. The settings widget appears below the handler's row.

### Security Model

The following safeguards are enforced when loading plugins:

- **URL pattern whitelisting** -- A plugin only receives URLs that match its declared `urlPatterns`. URLs that do not match are never passed to the plugin.
- **Reserved ID rejection** -- Plugins that declare a `handlerId` matching a built-in handler (`steam`, `udemy`, `amazon`, `generic`) are rejected at load time.
- **Overly broad pattern rejection** -- Patterns such as `*`, `http://*`, or `https://*` are rejected to prevent a plugin from intercepting all URLs.
- **ABI validation** -- `QPluginLoader` verifies Qt ABI compatibility before loading, preventing crashes from mismatched Qt versions.

---

## JSON Config Sources (GenericWebHandler)

For non-developers, PriceBell supports adding custom price sources via the `sources` database table without writing any code.

A JSON config source uses `GenericWebHandler`, which works as follows:

1. Substitutes the product URL into a URL template.
2. Makes an HTTP GET request to the resulting URL.
3. Extracts price and discount values from the JSON response using dot-notation paths.

### SourceConfig Fields

| Field | Type | Description |
|---|---|---|
| `id` | `string` | Unique identifier for this source. |
| `name` | `string` | Display name shown in the UI. |
| `urlTemplate` | `string` | API URL with a `{url}` placeholder, e.g. `"https://api.example.com/price?q={url}"`. |
| `pricePath` | `string` | Dot-notation path to the price value in the JSON response, e.g. `"data.price.amount"`. |
| `discountPath` | `string` | Dot-notation path to the discount value (optional). |
| `isDeveloperPlugin` | `bool` | Set to `false` for config sources. |

### Example

Given an API that returns the following JSON:

```json
{
    "data": {
        "price": {
            "amount": 29.99
        },
        "discount": 15
    }
}
```

Configure the source with:

- `pricePath`: `"data.price.amount"` -- resolves to `29.99`
- `discountPath`: `"data.discount"` -- resolves to `15`

The `GenericWebHandler` traverses the JSON object using each segment of the dot-notation path to reach the target value.
