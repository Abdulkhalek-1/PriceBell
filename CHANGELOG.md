# Changelog

All notable changes to PriceBell are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.4.0] - 2026-05-03

### Added
- **Dual-layout product list** — switch between the new card view (default for fresh installs) and the slimmer 5-column table view via a segmented toggle in the toolbar; choice persists across launches.
- **Right-hand detail pane** for the table view — shows full product info, alert conditions, check interval, and recent alert history when a row is selected. Splitter handle lets you collapse it.
- **URL auto-detect on Add Product** — paste a Steam, Udemy, or Amazon URL and the source is selected automatically. The product name is fetched in the background and prefilled (only if you haven't typed your own).
- **Single-instance enforcement** via `QLocalServer` — launching the app a second time raises the existing window instead of opening a duplicate.
- **Steam region selector** in Settings → General — defaults to your system locale so prices match what Steam actually charges you. Override with a curated list of common regions.
- **Feature-announcement framework** — one-time toasts introduce new features after an upgrade. Used in v1.4.0 to introduce the layout switcher, URL auto-detect, and Steam region.
- New `IPriceHandler::canHandle()` and `IPriceHandler::clone()` virtuals so plugins can participate in URL routing and safe background fetches.
- New `PluginManager::findHandlerForUrl()` for centralized URL → handler routing.
- Two new test suites: `test_url_routing` and `test_alert_dedup` (11 cases total).

### Changed
- **Notification dedup** — only one tray notification per product per app launch. The DB still records every triggered match so Alert History stays accurate. Re-arms when conditions are edited or the alert is dismissed.
- **Table slimmed from 8 → 5 columns**: Product (with source icon and URL tooltip), Price (right-aligned), Discount, Status, Last Checked. Conditions and Interval moved to the detail pane.
- **Bundled Noto Sans font** is now first in the QSS `font-family` list so Windows actually picks it up instead of falling back to Segoe UI.
- `FetchResult` extended with `name`, `currency`, and `originalPrice` fields so handlers can surface richer info to the UI.
- App version bumped to `1.4.0`.

### Fixed
- **Multi-select Remove** now deletes every selected product. Previously only the focused row was removed and the rest were silently skipped.
- **Steam shows the wrong price** — `cc=us` was hardcoded, so users outside the US saw USD prices that didn't match their actual Steam store. Region is now derived from the system locale or an explicit Settings override.
- **Mojibake on Windows** — non-ASCII string literals (em-dash, ≤, ≥, ✓, …) rendered as garbage on Windows because MSVC defaults to the system code page. Added `/utf-8` to MSVC compile flags.
- Steam parser now falls back to `initial` when `final` is missing (was silently returning $0 for some regions).

## [1.3.0] - 2026-04-04

### Added
- **CPack packaging** — DEB / RPM for Linux, NSIS installer + portable ZIP for Windows (x64 and x86).
- **CI release pipeline** — GitHub Actions builds Linux AppImage / DEB / RPM and Windows ZIP / NSIS installers, then publishes to GitHub Releases.
- **Auto-updater** — checks GitHub Releases on startup and via Help menu; `UpdateDownloader` + `UpdateDialog` show progress, support cancel, and launch the installer with UAC elevation on Windows.
- **Notification deep-links** — clicking a price-alert notification opens the product URL in the browser.
- **Bundled Noto Sans / Noto Naskh Arabic** fonts so Linux and Windows render the UI consistently.
- Launch-minimized-to-tray option (`--minimized` flag and Settings checkbox).
- Plugin starter example at `examples/plugin-starter/` with `IPlugin2` skeleton and README.
- Windows `VERSIONINFO` resource and macOS bundle metadata.

### Changed
- Update checker filters out pre-releases.

### Fixed
- Update download follows HTTP redirects (GitHub CDN serves a 302 to the actual asset).
- Windows installer launch uses `QDesktopServices::openUrl` so the UAC prompt fires correctly.
- Fullscreen state restored after closing to tray and reopening.

[1.4.0]: https://github.com/Abdulkhalek-1/PriceBell/releases/tag/v1.4.0
[1.3.0]: https://github.com/Abdulkhalek-1/PriceBell/releases/tag/v1.3.0
