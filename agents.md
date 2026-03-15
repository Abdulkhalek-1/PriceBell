# PriceBell — Agent Context

## Project Overview

PriceBell is a **C++17 desktop application** (Qt5) that monitors product prices across platforms (Steam, Udemy, Amazon, generic web) and alerts users when price/discount thresholds are met. It runs in the system tray for background monitoring. It is **not** a web app or backend service.

---

## Tech Stack

| Layer    | Technology                                                  |
|----------|-------------------------------------------------------------|
| Language | C++17                                                       |
| GUI      | Qt5 (Widgets, Sql, Network, Svg, LinguistTools)             |
| Build    | CMake 3.16+                                                 |
| Database | SQLite via Qt5 QSqlDatabase                                 |
| Testing  | Custom test targets (test_price_checker, test_repository)   |
| i18n     | Qt Linguist (English, Arabic, French)                       |
| License  | MIT                                                         |

---

## Directory Structure

```
PriceBell/
├── assets/
│   ├── icons/               # Tray icons (tray_normal.svg, tray_alert.svg)
│   ├── themes/              # Stylesheets (dark.qss)
│   ├── banner.svg           # Branding
│   ├── logo.svg             # App icon
│   ├── readme_banner.svg
│   └── resources.qrc        # Qt resource file
├── docs/                    # Architecture, building, handlers, plugins, user guide
├── i18n/                    # Translation files (.ts)
│   ├── pricebell_en.ts
│   ├── pricebell_ar.ts
│   └── pricebell_fr.ts
├── include/
│   ├── core/
│   │   ├── DataStructs.hpp      # Product, FetchResult, AlertEvent, SourceType, RESTART_EXIT_CODE
│   │   ├── IApplication.hpp     # App interface
│   │   ├── IPriceHandler.hpp    # Strategy interface for price sources
│   │   ├── IPlugin.hpp          # Plugin interface for native .so/.dll extensions
│   │   ├── AlertManager.hpp     # Price-drop notification engine
│   │   ├── PluginManager.hpp    # Dynamic plugin loader
│   │   ├── PriceChecker.hpp     # Filter matching logic
│   │   ├── PricePoller.hpp      # Background polling thread (+ checkNow slot)
│   │   └── SteamHandler.hpp     # Legacy Steam handler (see handlers/)
│   ├── gui/
│   │   ├── MainWindow.hpp       # Main window with product table, Check Now, update checker
│   │   ├── ProductDialog.hpp    # Add/edit product dialog
│   │   ├── AlertHistoryDialog.hpp # View past alerts
│   │   ├── SettingsDialog.hpp   # Settings with auto-start, updates, language, API keys
│   │   ├── TrayIcon.hpp         # System tray with context menu
│   │   └── Widgets.hpp          # Widget helpers
│   ├── handlers/
│   │   ├── AmazonHandler.hpp
│   │   ├── GenericWebHandler.hpp
│   │   ├── SteamHandler.hpp
│   │   └── UdemyHandler.hpp
│   ├── storage/
│   │   ├── Database.hpp         # SQLite connection & schema migrations
│   │   ├── ProductRepository.hpp
│   │   └── AlertRepository.hpp
│   └── utils/
│       ├── AutoStartManager.hpp # OS-level auto-start (Linux .desktop, macOS plist, Win registry)
│       ├── HttpClient.hpp       # Async HTTP via QNetworkAccessManager
│       ├── Logger.hpp           # File + console logger
│       └── UpdateChecker.hpp    # GitHub Releases API update checker
├── src/                         # Implementations mirror include/ layout
│   ├── core/
│   ├── gui/
│   ├── handlers/
│   ├── storage/
│   ├── utils/
│   └── main.cpp                 # Entry point (restart loop, DB init, i18n, RTL, tray)
├── tests/
│   ├── test_main.cpp
│   ├── test_price_checker.cpp
│   └── test_repository.cpp
├── CMakeLists.txt
├── README.md
├── LICENSE
└── CONTRIBUTING.md
```

---

## Core Data Model

Defined in `include/core/DataStructs.hpp`:

```cpp
static constexpr int RESTART_EXIT_CODE = 1000; // Used by main() restart loop

enum class ConditionType { PRICE_LESS_EQUAL, DISCOUNT_GREATER_EQUAL };
enum class SourceType    { STEAM, UDEMY, AMAZON, GENERIC, PLUGIN };
enum class AlertStatus   { TRIGGERED, DISMISSED };

struct PriceCondition { int id; ConditionType type; float value; };

struct Product {
    int id; std::string name; std::string url;
    SourceType source; std::string sourcePluginId;
    float currentPrice; float discount; bool isActive;
    std::vector<PriceCondition> filters;
    std::chrono::seconds checkInterval;
    std::chrono::system_clock::time_point lastChecked;
};

struct AlertEvent {
    int id; int productId; std::string productName;
    float priceAtTrigger; float discountAtTrigger;
    std::chrono::system_clock::time_point triggeredAt;
    AlertStatus status;
};

struct FetchResult { bool success; float price; float discount; std::string errorMsg; };
struct SourceConfig { std::string id, name, urlTemplate, pricePath, discountPath; bool isDeveloperPlugin; };
```

All types registered with `Q_DECLARE_METATYPE` for cross-thread signal delivery.

---

## Architecture Patterns

- **Strategy Pattern**: `IPriceHandler` interface with concrete handlers (`SteamHandler`, `AmazonHandler`, `UdemyHandler`, `GenericWebHandler`). Each handler implements `fetchProduct(url)`, `handlerId()`, `displayName()`.
- **Plugin System**: `IPlugin` interface for native `.so`/`.dll` extensions loaded at runtime by `PluginManager`.
- **Repository Pattern**: `ProductRepository` and `AlertRepository` abstract SQLite persistence behind CRUD interfaces.
- **Background Polling**: `PricePoller` runs on a `QThread`, periodically fetches prices, emits signals on changes.
- **Alert Engine**: `AlertManager` evaluates `PriceCondition` filters against `FetchResult` and fires `AlertEvent` signals.
- **MVC Separation**: `core/` + `handlers/` + `storage/` = model/controller, `gui/` = view, `utils/` = cross-cutting.
- **System Tray**: App minimizes to tray on close; `TrayIcon` shows desktop notifications and context menu.
- **Restart Loop**: `main()` wraps `QApplication` in a `do...while(exitCode == RESTART_EXIT_CODE)` loop. Language changes or File > Restart trigger a clean restart.
- **Database Migrations**: Versioned migrations via `schema_version` table. Each migration is a lambda in a `QVector`, executed sequentially in transactions.

---

## Key Components

### `PriceChecker` — `src/core/PriceChecker.cpp`

`isMatch(product)` — returns true if the product satisfies **all** `PriceCondition` filters.

### Handlers — `src/handlers/`

| Handler | Source | Status |
| ------- | ------ | ------ |
| `SteamHandler` | Steam store API | Implemented |
| `UdemyHandler` | Udemy API | Implemented |
| `AmazonHandler` | Amazon product pages | Implemented |
| `GenericWebHandler` | Any URL + CSS/JSON selector | Implemented |

All implement `IPriceHandler::fetchProduct(url) → FetchResult`.

### `PricePoller` — `src/core/PricePoller.cpp`

Runs on a background `QThread`. Iterates active products by check interval, calls the appropriate handler, emits `priceChanged(productId, price, discount)`. Also provides `checkNow(productId)` slot for on-demand price checks invoked via `QMetaObject::invokeMethod` with `Qt::QueuedConnection`.

### `AlertManager` — `src/core/AlertManager.cpp`

Listens to price changes, evaluates filters via `PriceChecker::isMatch()`, persists `AlertEvent` via `AlertRepository`, emits `alertTriggered(AlertEvent)`.

### `PluginManager` — `src/core/PluginManager.cpp`

Scans `plugins/` directory for `.so`/`.dll` files implementing `IPlugin`. Registers them as additional `IPriceHandler` sources.

### Storage — `src/storage/`

- `Database` — Opens/creates SQLite DB, applies versioned schema migrations via `schema_version` table. Each migration runs in a transaction.
- `ProductRepository` — CRUD for `Product` records.
- `AlertRepository` — CRUD for `AlertEvent` records.

### `MainWindow` — `src/gui/MainWindow.cpp`

Product table with columns (Name, Source, Price, Discount, Status, Last Checked, Interval). Toolbar: Add, Edit, Remove, Check Now, Alert History, Settings. Menu bar: File (Add/Edit/Remove/Restart/Quit), View (Alert History), Tools (Check Now, Settings), Help (Check for Updates, About). Supports multi-select for batch Check Now. Integrates tray icon, poller thread, alert manager, and update checker.

### `ProductDialog` — `src/gui/ProductDialog.cpp`

Modal dialog for adding/editing products. Source selector (Steam/Udemy/Amazon/Generic/Plugin), URL, filter list with condition type/value, check interval.

### `SettingsDialog` — `src/gui/SettingsDialog.cpp`

Groups: Startup (auto-start), Updates (check on startup), Udemy API, Amazon API, Polling, Plugin Directory, Language. Saves to `QSettings`. Language change triggers app restart via `RESTART_EXIT_CODE`. Reports `isRestartNeeded()` to `MainWindow`.

### `AlertHistoryDialog` — `src/gui/AlertHistoryDialog.cpp`

Table of past `AlertEvent` records with dismiss/clear actions.

### `TrayIcon` — `src/gui/TrayIcon.cpp`

System tray icon with context menu (Show, Mute Notifications, Quit). Shows balloon notifications on alerts. Swaps icon between normal/alert states.

### `UpdateChecker` — `src/utils/UpdateChecker.cpp`

Checks GitHub Releases API (`Abdulkhalek-1/PriceBell`) for newer versions. Compares semantic versions. Signals: `updateAvailable(version, url)`, `noUpdateAvailable()`, `checkFailed(errorMsg)`. Used by `MainWindow` for both startup auto-check and manual "Check for Updates".

### `AutoStartManager` — `src/utils/AutoStartManager.cpp`

Platform-specific auto-start management. Linux: `.desktop` file in `~/.config/autostart/`. macOS: plist in `~/Library/LaunchAgents/`. Windows: registry entry in `HKCU\...\Run`.

### `HttpClient` — `src/utils/HttpClient.cpp`

Async HTTP GET via `QNetworkAccessManager`. Used by handlers and `UpdateChecker`.

### `Logger` — `src/utils/Logger.cpp`

`Logger::info()`, `Logger::error()`, `Logger::debug()` — writes to console and rotating log file.

---

## Current State

| Area | Status |
|------|--------|
| Qt GUI (product CRUD, table, dialogs) | **Implemented** |
| Filter logic (`PriceChecker::isMatch`) | **Implemented** |
| SQLite persistence (products, alerts) | **Implemented** |
| Database migrations (`schema_version`) | **Implemented** |
| Price handlers (Steam, Udemy, Amazon, Generic) | **Implemented** |
| Background polling (`PricePoller`) | **Implemented** |
| Check Now (on-demand, multi-select) | **Implemented** |
| Alert engine (`AlertManager`) | **Implemented** |
| System tray + notifications | **Implemented** |
| Settings dialog (startup, updates, API, lang) | **Implemented** |
| Alert history dialog | **Implemented** |
| Plugin system | **Implemented** |
| Auto-update checker (GitHub Releases API) | **Implemented** |
| App restart system (exit-code loop) | **Implemented** |
| Auto-start (Linux, macOS, Windows) | **Implemented** |
| i18n (EN, AR, FR) — 118 strings | **Implemented** |
| Dark theme (Catppuccin Mocha) | **Implemented** |
| RTL layout support | **Implemented** |
| Tests (price checker, repository) | **Implemented** |
| CI/CD (GitHub Actions: Linux AppImage, Windows) | **Implemented** |

---

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make
./PriceBell
```

Requires: `cmake 3.16+`, `g++` (C++17), Qt5 modules: `Widgets`, `Sql`, `Network`, `Svg`, `LinguistTools`. CMake defines `APP_VERSION` from `project(PriceBell VERSION x.y.z)` and passes it to the compiler.

---

## Coding Conventions

- Headers in `include/<layer>/`, implementations in `src/<layer>/`
- Interfaces prefixed with `I` (`IApplication`, `IPriceHandler`, `IPlugin`)
- `.hpp` for headers, `.cpp` for implementations
- Qt slots/signals follow Qt5 conventions (`Q_OBJECT`, `signals:`, `public slots:`)
- Member variables prefixed with `m_` (e.g., `m_table`, `m_poller`)
- Use `Logger::info()` / `Logger::error()` instead of raw `std::cout`
- Types registered with `Q_DECLARE_METATYPE` for cross-thread signals
- All handlers implement `IPriceHandler` with `handlerId()` and `displayName()`

---

## Extension Points

### Adding a new price handler

1. Create `include/handlers/<Name>Handler.hpp` implementing `IPriceHandler`
2. Create `src/handlers/<Name>Handler.cpp`
3. Add `SourceType` enum value in `DataStructs.hpp`
4. Register handler in `MainWindow::setupPoller()` dispatch
5. Add source option to `ProductDialog`'s source combo box

### Adding a native plugin

1. Implement `IPlugin` interface in a shared library (`.so` / `.dll`)
2. Export the `create_plugin()` factory function
3. Drop the library into the `plugins/` directory — `PluginManager` loads it at startup

### Adding a new language

1. Create `i18n/pricebell_<locale>.ts`
2. Add the `.ts` file to the `TS_FILES` list in `CMakeLists.txt`
3. Add locale option in `SettingsDialog`
