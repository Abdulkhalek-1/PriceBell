# PriceBell User Guide

## Getting Started

### Installation

Build PriceBell from source by following the instructions in [BUILDING.md](BUILDING.md). Pre-built binaries may be available on the [Releases](https://github.com/Abdulkhalek-1/PriceBell/releases) page.

### First Launch

When you start PriceBell for the first time:

- The main window opens with an empty product table.
- A system tray icon appears in your desktop notification area.
- The Catppuccin Mocha dark theme is applied by default.

## Adding a Product

1. Click the **Add Product** button in the toolbar, or go to **File > Add Product**.
2. Fill in the product details in the dialog that appears:

| Field | Description |
| --- | --- |
| Name | A label for your own reference |
| URL | The product page URL |
| Source | Select one of: Steam, Udemy, Amazon, Generic, or a plugin source |

### Source-specific URL formats

- **Steam**: Paste a Steam store URL such as `https://store.steampowered.com/app/730/Counter-Strike_2/`. Both `https://` and `http://` are accepted, including age-check URLs like `https://store.steampowered.com/agecheck/app/730/`.
- **Udemy**: Paste a course URL such as `https://www.udemy.com/course/my-course/`. URLs with or without `www.` and both `http://` and `https://` are accepted.
- **Amazon**: Paste a product page URL. Amazon tracking requires PA API credentials configured in Settings (see below).
- **Generic**: Used for custom sources added through the plugin system or JSON config sources.

## Alert Conditions

Each product can have one or more alert conditions. You configure these in the Add/Edit Product dialog.

Available condition types:

- **Price <= threshold** -- triggers when the product's price drops to or below the specified value.
- **Discount >= threshold** -- triggers when the product's discount percentage reaches or exceeds the specified value.

When multiple conditions are attached to a single product, **all conditions must be met simultaneously** (AND logic) for an alert to fire.

## Check Interval

Each product has its own polling interval that controls how frequently PriceBell checks for price updates.

- The interval can be set anywhere from **30 seconds** to **86400 seconds** (24 hours).
- A default interval can be configured in Settings and is applied to new products automatically.
- You can override the interval per product when adding or editing it.

## Product Table

The main window displays all tracked products in a table with the following columns:

| Column | Description |
| --- | --- |
| Name | Product label |
| Source | Handler used (Steam, Udemy, Amazon, Generic, or plugin name) with source icon |
| Current Price | Last fetched price (formatted with currency) |
| Discount % | Last fetched discount percentage |
| Status | Watching, Paused, Checking..., Error, or Alert! with status icon |
| Last Checked | Timestamp of the most recent price fetch |
| Conditions | Alert conditions configured for this product |
| Interval (s) | Polling interval in seconds |

## Check Now

You can manually trigger an immediate price check without waiting for the next polling interval:

1. **Select products** in the table using click, Ctrl+click, or Shift+click for multi-select.
2. Click the **Check Now** button in the toolbar, or go to **Tools > Check Now**.
3. If no products are selected, all products are checked.
4. While checking, the Status column shows **Checking...** in yellow. On completion it returns to **Watching** or shows **Error** if the fetch failed.

## Alerts and Notifications

When all conditions for a product are met:

- A **desktop notification** is shown via the system tray.
- A **notification sound** is played (if enabled in Settings).
- The corresponding row in the product table is **highlighted** with an alert icon.

### Alert History

To review past alerts:

- Go to **View > Alert History**, or click the **Alert History** button in the toolbar.
- The Alert History dialog lists all triggered alerts with the product name, price and discount at the time of trigger, and timestamp.
- You can **dismiss** individual alerts from the history dialog.

## System Tray

PriceBell runs in the background via the system tray, allowing it to continue monitoring prices after the window is closed.

- **Closing the window** minimizes the application to the tray instead of quitting.
- **Double-click** the tray icon to restore the main window.
- **Right-click** the tray icon for a context menu with the following options:
  - **Show PriceBell** -- restore the main window.
  - **Mute Notifications** -- temporarily suppress desktop notifications.
  - **Quit** -- fully exit the application.
- The tray icon changes appearance when there are active (untriggered) alerts.

## Settings

Open the settings dialog by clicking the **Settings** button in the toolbar, or go to **Tools > Settings**. Settings are organized into three tabs.

### General Tab

**Startup:**

| Field | Description |
| --- | --- |
| Open on Startup | Launch PriceBell automatically when your system starts |

**Updates:**

| Field | Description |
| --- | --- |
| Check for updates on startup | Automatically check GitHub for new releases when the app launches |

You can also manually check via **Help > Check for Updates**. If a newer version is available, you will be offered a link to the release page.

**Notifications:**

| Field | Description |
| --- | --- |
| Play notification sound | Enable or disable alert sounds |
| Sound file path | Path to a custom `.wav` file, or leave empty for the built-in bell |
| Browse | Select a `.wav` file from your filesystem |
| Test | Preview the selected notification sound |
| Reset | Reset to the built-in default sound |

**Polling:**

| Field | Description |
| --- | --- |
| Default Check Interval | The default polling interval (in seconds) applied to new products |

**Language:**

Select from English, Arabic, or French. Changing the language automatically restarts the application to apply the new locale. Arabic enables right-to-left (RTL) layout.

### API Keys Tab

**Udemy API:**

| Field | Description |
| --- | --- |
| Client ID | Your Udemy API client ID |
| Client Secret | Your Udemy API client secret |

Obtain these from the [Udemy API](https://www.udemy.com/developers/) page.

**Amazon Product Advertising API:**

| Field | Description |
| --- | --- |
| Access Key | Your Amazon PA API access key |
| Secret Key | Your Amazon PA API secret key |
| Partner Tag | Your Amazon Associates partner tag |

These are required to track Amazon product prices. Sign up through [Amazon Associates](https://affiliate-program.amazon.com/).

### Plugins Tab

**Plugin Directory:** Path to the directory where PriceBell looks for native plugins. Click **Browse** to select a directory.

**Registered Handlers:** A list of all loaded handlers (both built-in and plugin), showing their name, ID, source icon, and a badge indicating whether they are built-in or plugin-provided. The footer shows a count of built-in vs. plugin handlers.

Plugins that implement the `IPlugin2` interface may display additional settings widgets inline below their handler row.

## Restarting

You can restart PriceBell at any time via **File > Restart**. The app also restarts automatically when you change the language in Settings.
