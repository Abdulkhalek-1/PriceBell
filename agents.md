# PriceBell — Agent Context

## Project Overview

PriceBell is a **C++17 desktop application** (Qt5) that monitors product prices across platforms (Steam, Udemy) and alerts users when price/discount thresholds are met. It is **not** a web app or backend service.

---

## Tech Stack

| Layer       | Technology         |
|-------------|--------------------|
| Language    | C++17              |
| GUI         | Qt5 (QMainWindow, QDialog, QTableWidget) |
| Build       | CMake 3.10+        |
| Testing     | Placeholder (tests/test_main.cpp) |
| License     | MIT                |

---

## Directory Structure

```
PriceBell/
├── assets/                  # App assets (logo.png)
├── include/
│   ├── core/                # Business logic headers
│   │   ├── DataStructs.hpp  # Product & PriceCondition types
│   │   ├── IApplication.hpp # App interface
│   │   ├── IPriceHandler.hpp# Price source strategy interface
│   │   └── SteamHandler.hpp # Steam implementation header
│   ├── gui/                 # Qt GUI headers
│   │   ├── MainWindow.hpp
│   │   ├── ProductDialog.hpp
│   │   └── Widgets.hpp
│   └── utils/
│       └── Logger.hpp       # Static console logger
├── src/
│   ├── core/
│   │   ├── Application.cpp  # STUB — empty
│   │   ├── DataProcessor.cpp# STUB — empty
│   │   ├── PriceChecker.cpp # isMatch() filter logic
│   │   └── SteamHandler.cpp # Mocked — returns hardcoded product
│   ├── gui/
│   │   ├── MainWindow.cpp   # Main window + product table
│   │   ├── ProductDialog.cpp# Add-product dialog
│   │   └── Widgets.cpp      # Stub widget helpers
│   ├── utils/
│   │   └── Logger.cpp       # log() implementation
│   └── main.cpp             # QApplication entry point
├── tests/
│   └── test_main.cpp        # Placeholder test runner
├── CMakeLists.txt
├── README.md
└── LICENSE
```

---

## Core Data Model

Defined in [include/core/DataStructs.hpp](include/core/DataStructs.hpp):

```cpp
enum ConditionType { PRICE_GREATER_EQUAL, DISCOUNT_GREATER_EQUAL };

struct PriceCondition {
    ConditionType type;
    float value;
};

struct Product {
    int id;
    std::string name;
    std::string source;       // "steam" | "udemy"
    float currentPrice;
    float discount;
    std::vector<PriceCondition> filters;
    std::chrono::seconds checkInterval;
    std::chrono::system_clock::time_point lastChecked;
};
```

---

## Architecture Patterns

- **Strategy Pattern**: `IPriceHandler` interface allows plugging in different price sources. `SteamHandler` is the only implementation so far.
- **MVC-ish separation**: `core/` handles business logic, `gui/` handles UI, `utils/` handles cross-cutting concerns.
- **In-memory only**: No database or persistence. All product data lives in a `std::vector<Product>` for the session lifetime.

---

## Key Components

### `PriceChecker` — [src/core/PriceChecker.cpp](src/core/PriceChecker.cpp)
`isMatch(product)` — returns true if the product satisfies **all** `PriceCondition` filters.

### `SteamHandler` — [src/core/SteamHandler.cpp](src/core/SteamHandler.cpp)
Implements `IPriceHandler::fetchProduct(url)`. Currently **mocked** — returns a hardcoded Half-Life 3 product at $59.99 with 10% discount.

### `MainWindow` — [src/gui/MainWindow.cpp](src/gui/MainWindow.cpp)
6-column product table (ID, Name, Source, Price, Discount, Check Interval). Add/Remove buttons. Menu bar mirrors these actions.

### `ProductDialog` — [src/gui/ProductDialog.cpp](src/gui/ProductDialog.cpp)
Modal dialog: name, URL, source (Steam/Udemy), filter list, filter type/value spinbox, check interval (30s–86400s). Returns a `Product` on accept.

### `Logger` — [src/utils/Logger.cpp](src/utils/Logger.cpp)
`Logger::log(message)` — prefixes with `[LOG]` and writes to stdout.

---

## Current State & Known Gaps

| Area | Status |
|------|--------|
| Qt GUI (add/remove/display products) | Working |
| Filter logic (`isMatch`) | Working |
| Price fetching (SteamHandler) | **Mocked** |
| Persistence / database | **Missing** |
| Notification / alert system | **Missing** |
| Background polling / scheduler | **Missing** |
| Application.cpp | **Empty stub** |
| DataProcessor.cpp | **Empty stub** |
| Tests | **Empty placeholder** |
| Udemy handler | **Not implemented** |

---

## Build Instructions

```bash
mkdir build && cd build
cmake ..
make
./PriceBell
```

Requires: `cmake`, `g++` (C++17), `Qt5 Widgets`.

---

## Coding Conventions

- Headers in `include/<layer>/`, implementations in `src/<layer>/`
- Interfaces prefixed with `I` (`IApplication`, `IPriceHandler`)
- `.hpp` for headers, `.cpp` for implementations
- Qt slots/signals follow Qt5 conventions (`Q_OBJECT`, `signals:`, `public slots:`)
- Static utility methods (see `Logger::log`)
- Avoid adding raw `std::cout` — use `Logger::log` instead

---

## Extension Points

When adding a new price source (e.g., Udemy, Epic Games):
1. Create `include/core/<Name>Handler.hpp` implementing `IPriceHandler`
2. Create `src/core/<Name>Handler.cpp`
3. Add source option to `ProductDialog`'s source `QComboBox`
4. Register handler in application dispatch logic

When adding persistence:
- Consider SQLite via Qt's `QSqlDatabase` / `QSqlite` driver (no external dep)
- `Product` struct maps cleanly to a single table

When adding notifications:
- Qt's `QSystemTrayIcon` is the natural fit for desktop alerts
- Hook into the background polling loop once `Application.cpp` is implemented