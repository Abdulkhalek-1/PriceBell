# Adding Built-in Price Handlers

## When to Add a Built-in Handler vs. a Plugin

- **Built-in handler**: Use this for major, well-known platforms that benefit from tight integration and should ship with every copy of PriceBell (e.g., Steam, Udemy, Amazon).
- **Plugin**: Use this for niche or user-specific sources that do not need to be included in the main distribution. See [PLUGINS.md](PLUGINS.md) for plugin development.

## Existing Handlers

The following built-in handlers serve as reference implementations:

| Handler | Platform | Approach |
| --- | --- | --- |
| `SteamHandler` | Steam Store | Uses the `/api/appdetails` endpoint. Extracts the app ID from the URL via regex. |
| `UdemyHandler` | Udemy | Uses Udemy API v2.0 with HTTP Basic Auth (client credentials). |
| `AmazonHandler` | Amazon | Uses the Product Advertising API 5.0. Extracts the ASIN from the URL. |
| `GenericWebHandler` | Configurable | Template-based URL substitution with dot-notation JSON path extraction. |

## Step-by-Step Guide

### 1. Create the Header

Create `include/handlers/MyHandler.hpp`:

```cpp
#pragma once
#include "core/IPriceHandler.hpp"

class HttpClient;

class MyHandler : public IPriceHandler {
public:
    bool validateUrl(const std::string& url) const override;
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId() const override;
    std::string displayName() const override;
    void setHttpClient(HttpClient* http) override { m_http = http; }

private:
    HttpClient* m_http = nullptr;
};
```

### 2. Create the Implementation

Create `src/handlers/MyHandler.cpp`:

Implement `fetchProduct()` using the injected `HttpClient` for HTTP requests. The `HttpClient*` is set by `PluginManager` via `setHttpClient()` before `fetchProduct()` is called -- do not create your own `HttpClient` instance.

```cpp
#include "handlers/MyHandler.hpp"
#include "utils/HttpClient.hpp"
#include "utils/Logger.hpp"
#include <QJsonDocument>
#include <QJsonObject>

std::string MyHandler::handlerId() const {
    return "myhandler";
}

std::string MyHandler::displayName() const {
    return "My Handler";
}

bool MyHandler::validateUrl(const std::string& url) const {
    return url.find("https://mystore.com/product/") == 0;
}

FetchResult MyHandler::fetchProduct(const std::string& url) {
    FetchResult result;

    if (!validateUrl(url)) {
        result.errorMsg = "Invalid URL for this handler";
        return result;
    }

    auto response = m_http->getSync(
        "https://api.mystore.com/price?url=" + url);

    if (!response.success) {
        result.errorMsg = "HTTP request failed: " + response.error;
        return result;
    }

    // Parse JSON response and extract price/discount
    QJsonDocument doc = QJsonDocument::fromJson(
        QByteArray::fromStdString(response.body));
    QJsonObject obj = doc.object();

    result.price = static_cast<float>(obj["price"].toDouble());
    result.discount = static_cast<float>(obj["discount"].toDouble());
    result.success = true;
    return result;
}
```

### 3. Register in PluginManager

In `PluginManager::registerBuiltins()`, add the new handler:

```cpp
registerHandler("myhandler", std::make_shared<MyHandler>());
```

Include the header at the top of the file:

```cpp
#include "handlers/MyHandler.hpp"
```

### 4. Add Source Type to DataStructs.hpp

Add a new value to the `SourceType` enum in `include/core/DataStructs.hpp`:

```cpp
enum class SourceType {
    Steam = 0,
    Udemy = 1,
    Amazon = 2,
    Generic = 3,
    Plugin = 4,
    MyHandler = 5  // <-- add this
};
```

### 5. Add to ProductDialog UI

In `src/gui/ProductDialog.cpp`, add the new source name to the source `QComboBox` so users can select it when adding a product:

```cpp
m_sourceCombo->addItem("My Handler");
```

Ensure the combo box index aligns with the `SourceType` enum value.

### 6. Add Source Icon

Create an SVG icon for the new source at `assets/icons/source_myhandler.svg` (24x24, Catppuccin stroke style). Register it in `assets/resources.qrc`:

```xml
<file>icons/source_myhandler.svg</file>
```

Then reference it in `MainWindow::refreshTable()` and `SettingsDialog::createPluginsTab()` for the new source type.

### 7. Pre-seed in Database Schema

Add an `INSERT` statement for the new source in `Database::applySchema()`:

```sql
INSERT OR IGNORE INTO sources (id, name, type) VALUES ('myhandler', 'My Handler', 5);
```

This ensures the source exists in the database on first run and after schema migrations.

### 8. Update CMakeLists.txt

The project uses `file(GLOB_RECURSE)` to collect source files, so new files placed in `src/handlers/` and `include/handlers/` are picked up automatically. No CMake changes are needed.

### 9. Add Source Mapping

Update the mapping functions in `ProductRepository` to handle the new source type:

In `ProductRepository::sourceIdToType()`:

```cpp
if (id == "myhandler") return SourceType::MyHandler;
```

In `ProductRepository::sourceTypeToId()`:

```cpp
if (type == SourceType::MyHandler) return "myhandler";
```

These functions translate between the string IDs stored in the database and the `SourceType` enum used in application code.

### 10. Add Translations

Add `tr()` strings for the handler's display name in `MainWindow::refreshTable()` and add corresponding entries to all three `.ts` files in `i18n/`.
