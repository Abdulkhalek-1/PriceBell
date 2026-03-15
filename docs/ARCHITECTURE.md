# PriceBell Architecture

## High-Level Layers

```
+----------------------------------+
|           GUI Layer              |
|  MainWindow, ProductDialog,     |
|  AlertHistoryDialog,            |
|  SettingsDialog, TrayIcon       |
+----------------------------------+
|          Core Layer              |
|  PriceChecker, PricePoller,     |
|  AlertManager, PluginManager    |
+----------------------------------+
|        Handlers Layer            |
|  SteamHandler, UdemyHandler,    |
|  AmazonHandler,                 |
|  GenericWebHandler              |
+----------------------------------+
|        Storage Layer             |
|  Database, ProductRepository,   |
|  AlertRepository                |
+----------------------------------+
|        Utils Layer               |
|  Logger, HttpClient,            |
|  UpdateChecker, AutoStartManager|
+----------------------------------+
```

## Directory Structure

```
include/
  core/       DataStructs.hpp, IPriceHandler.hpp, IPlugin.hpp,
              PriceChecker.hpp, PricePoller.hpp, AlertManager.hpp,
              PluginManager.hpp
  handlers/   SteamHandler.hpp, UdemyHandler.hpp, AmazonHandler.hpp,
              GenericWebHandler.hpp
  storage/    Database.hpp, ProductRepository.hpp, AlertRepository.hpp
  gui/        MainWindow.hpp, ProductDialog.hpp, AlertHistoryDialog.hpp,
              SettingsDialog.hpp, TrayIcon.hpp
  utils/      Logger.hpp, HttpClient.hpp, UpdateChecker.hpp,
              AutoStartManager.hpp

src/          Mirrors include/ structure with .cpp implementations
```

## Key Interfaces

### IPriceHandler

Abstract base class that every price-fetching handler must implement:

- `fetchProduct(url) -> FetchResult` -- fetches the current price and discount for the given URL.
- `handlerId() -> QString` -- returns a unique string identifier for the handler.
- `displayName() -> QString` -- returns a human-readable name shown in the UI.

### IPlugin

Extends `IPriceHandler` with plugin-specific functionality:

- `metadata() -> QJsonObject` -- returns plugin metadata (name, version, author, URL pattern).
- Declares the Qt plugin interface identifier `"com.pricebell.IPlugin/1.0"`.

## Data Model (DataStructs.hpp)

### Enums

| Enum | Values |
|------|--------|
| `ConditionType` | `PRICE_LESS_EQUAL`, `DISCOUNT_GREATER_EQUAL` |
| `SourceType` | `STEAM`, `UDEMY`, `AMAZON`, `GENERIC`, `PLUGIN` |
| `AlertStatus` | `TRIGGERED`, `DISMISSED` |

### Structs

- **Product** -- `id`, `name`, `url`, `source`, `currentPrice`, `discount`, `isActive`, `filters` (list of PriceCondition), `checkInterval`, `lastChecked`
- **PriceCondition** -- `id`, `type` (ConditionType), `value`
- **AlertEvent** -- `id`, `productId`, `productName`, `priceAtTrigger`, `discountAtTrigger`, `triggeredAt`, `status`
- **FetchResult** -- `success`, `price`, `discount`, `errorMsg`
- **SourceConfig** -- `id`, `name`, `urlTemplate`, `pricePath`, `discountPath`, `isDeveloperPlugin`

## SQLite Schema

The database contains five tables. Schema changes are managed via versioned migrations (see `schema_version` table).

### sources

| Column | Type | Notes |
|--------|------|-------|
| id | TEXT | Primary key |
| name | TEXT | |
| type | INTEGER | Maps to SourceType enum |
| url_template | TEXT | |
| price_path | TEXT | |
| discount_path | TEXT | |
| is_plugin | INTEGER | 0 or 1 |

Pre-seeded rows: `steam`, `udemy`, `amazon`.

### products

| Column | Type | Notes |
|--------|------|-------|
| id | INTEGER | Primary key, auto-increment |
| name | TEXT | |
| url | TEXT | |
| source_id | TEXT | Foreign key to sources |
| current_price | REAL | |
| discount | REAL | |
| is_active | INTEGER | 0 or 1 |
| check_interval | INTEGER | Seconds |
| last_checked | TEXT | ISO 8601 timestamp |

### price_conditions

| Column | Type | Notes |
|--------|------|-------|
| id | INTEGER | Primary key, auto-increment |
| product_id | INTEGER | Foreign key to products (CASCADE delete) |
| condition_type | INTEGER | Maps to ConditionType enum |
| value | REAL | |

### alert_history

| Column | Type | Notes |
|--------|------|-------|
| id | INTEGER | Primary key, auto-increment |
| product_id | INTEGER | Foreign key to products (CASCADE delete) |
| product_name | TEXT | |
| price_at_trigger | REAL | |
| discount_at_trigger | REAL | |
| triggered_at | TEXT | ISO 8601 timestamp |
| status | INTEGER | Maps to AlertStatus enum |

### schema_version

| Column | Type | Notes |
|--------|------|-------|
| version | INTEGER | Current schema version (single row) |

Used by `Database::applyMigrations()` to track which migrations have been applied. Each migration increments this value inside a transaction.

## Plugin System (Dual-Tier)

PriceBell supports three tiers of price handlers.

### Tier 1 -- Built-in Handlers

`SteamHandler`, `UdemyHandler`, and `AmazonHandler` are compiled into the application and registered via `PluginManager::registerBuiltins()`.

### Tier 2a -- Native Developer Plugins

Shared libraries (`.so` on Linux, `.dll` on Windows, `.dylib` on macOS) placed in the `plugins/` directory are loaded at runtime via `QPluginLoader`. Each plugin must implement the `IPlugin` interface. Plugin metadata is validated on load, and URL pattern sandboxing ensures a plugin can only handle URLs matching its declared pattern.

### Tier 2b -- JSON Config Sources

User-defined sources stored in the `sources` database table. Each entry specifies a `url_template`, `price_path`, and `discount_path`. At runtime these are instantiated as `GenericWebHandler` instances, enabling users to add new sources without writing code.

## Threading Model

- **PricePoller** runs on its own `QThread`.
- Each tracked product has a dedicated `QTimer` that fires at its configured check interval.
- The internal product map is protected by a `QMutex`.
- Cross-thread signal delivery is enabled by registering custom types with `qRegisterMetaType` for `Product`, `FetchResult`, and `AlertEvent`.

## Signal Flow

```
PricePoller::priceUpdated(Product, FetchResult)
    -> AlertManager::onPriceUpdated()
        -> PriceChecker::isMatch() evaluation
        -> AlertRepository::save(AlertEvent)
        -> AlertManager::alertTriggered(AlertEvent)
            -> MainWindow::onAlertTriggered()
            -> TrayIcon::showAlert()
```

When `PricePoller` emits a price update, `AlertManager` evaluates every condition attached to the product using `PriceChecker::isMatch()`. If all conditions are satisfied (AND logic), an `AlertEvent` is persisted to the database and the `alertTriggered` signal propagates to the GUI layer for display and notification.

## Settings

Application settings are managed via `QSettings` with the following keys:

| Key | Description |
|-----|-------------|
| `language` | User's selected locale (default: `en`) |
| `udemy/client_id` | Udemy API client ID |
| `udemy/client_secret` | Udemy API client secret |
| `amazon/access_key` | Amazon PA API access key |
| `amazon/secret_key` | Amazon PA API secret key |
| `amazon/partner_tag` | Amazon Associates partner tag |
| `polling/default_interval` | Default polling interval in seconds |
| `plugins/directory` | Path to the plugins directory |
| `updates/check_on_startup` | Auto-check GitHub for updates on startup (default: `true`) |

## App Restart

`main()` wraps the `QApplication` event loop in a `do...while(exitCode == RESTART_EXIT_CODE)` loop. When SettingsDialog detects a language change, it sets a restart flag. After the dialog closes, MainWindow calls `qApp->exit(RESTART_EXIT_CODE)`, which causes the loop to destroy the current `QApplication` and create a fresh one with the new locale loaded. The constant `RESTART_EXIT_CODE` (1000) is defined in `DataStructs.hpp`.

## Update Checker

`UpdateChecker` (in `utils/`) queries `https://api.github.com/repos/Abdulkhalek-1/PriceBell/releases/latest` and compares the remote `tag_name` (semantic version) against the compile-time `APP_VERSION` macro. On startup (if enabled in settings) it checks silently and shows a tray notification if an update is available. Manual checks via Help > Check for Updates show a dialog for all outcomes.

## Theme

The application ships with a Catppuccin Mocha dark theme defined in `assets/themes/dark.qss`. The stylesheet is compiled into the binary via the Qt resource system and loaded at startup from `:/assets/themes/dark.qss`.
