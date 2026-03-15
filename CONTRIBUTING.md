# Contributing to PriceBell

Thank you for your interest in contributing to PriceBell. This guide covers everything you need to get started.

## Getting Started

1. Fork and clone the repository:
   ```bash
   git clone https://github.com/<your-username>/PriceBell.git
   cd PriceBell
   ```

2. Build the project. See [docs/BUILDING.md](docs/BUILDING.md) for full build instructions.

3. Create a feature branch:
   ```bash
   git checkout -b feature/my-feature
   ```

## Code Style

Follow these conventions to keep the codebase consistent:

- **Standard**: C++17
- **Framework**: Qt5 conventions (`Q_OBJECT` macro, signals/slots)
- **Member variables**: `m_` prefix (e.g., `m_productTable`, `m_priceLabel`)
- **Class names**: PascalCase (e.g., `MainWindow`, `PriceChecker`)
- **Method names**: camelCase (e.g., `fetchProduct`, `isMatch`)
- **File extensions**: `.hpp` for headers, `.cpp` for implementations
- **Directory layout**: Headers in `include/<layer>/`, sources in `src/<layer>/`
- **Interfaces**: Prefixed with `I` (e.g., `IPriceHandler`, `IPlugin`, `IPlugin2`)
- **Logging**: Use `Logger::info()`, `Logger::warn()`, and `Logger::error()` instead of raw `std::cout` or `qDebug()`
- **Thread safety**: Use `QMetaObject::invokeMethod` with `Qt::QueuedConnection` when calling slots across threads (e.g., from AppController to PricePoller)
- **Dependency injection**: Handlers receive their `HttpClient*` via `setHttpClient()` — the `PluginManager` creates the client lazily on the calling thread to satisfy Qt's thread-affinity requirements
- **Icons**: Use SVG icons from `assets/icons/` via Qt resource paths (`:/assets/icons/...`) instead of emoji characters

## Commit Messages

Use conventional commit format:

```
type(scope): brief description

Optional longer description.
```

**Types:**

| Type | Usage |
|---|---|
| `feat` | A new feature |
| `fix` | A bug fix |
| `refactor` | Code restructuring without behavior change |
| `docs` | Documentation changes |
| `test` | Adding or updating tests |
| `chore` | Build config, dependencies, or other maintenance |

**Examples:**

```
feat(handlers): add Amazon price handler

fix(poller): prevent duplicate polling when interval is zero

docs: update plugin development guide
```

## Pull Request Workflow

1. **Ensure the project builds cleanly** with no warnings or errors.
2. **Run the test suite**:
   ```bash
   cd build && ctest --output-on-failure
   ```
3. **Keep PRs focused** -- one feature or one fix per pull request.
4. **Write a clear PR description** explaining what was changed and why.

## Adding Tests

- Test files go in `tests/`.
- Follow the existing patterns in `test_price_checker.cpp`, `test_repository.cpp`, `test_url_validation.cpp`, and `test_settings_provider.cpp`.
- Repository tests use in-memory SQLite (`:memory:`) to avoid filesystem dependencies.
- URL validation tests verify handler `validateUrl()` and `fetchProduct()` with invalid URLs.
- Add new test targets in `CMakeLists.txt` following the existing test target structure.

## Database Migrations

If your change adds or modifies SQLite tables, you must add a migration:

1. Open `src/storage/Database.cpp` and find the `applyMigrations()` function.
2. Append a new lambda to the `migrations` vector. The lambda receives a `QSqlQuery&` and returns `bool`.
3. Each migration upgrades from version N to N+1. The `schema_version` table tracks the current version.
4. Migrations run inside a transaction — if your lambda returns `false`, the transaction is rolled back.
5. Never modify existing migrations — only append new ones.

## Translations (i18n)

All user-facing strings must be wrapped in `tr()`. After adding new strings:

```bash
lupdate src/ include/ -ts i18n/pricebell_en.ts i18n/pricebell_ar.ts i18n/pricebell_fr.ts
```

Then provide translations for Arabic and French in the `.ts` files. English translations can be left empty (source text is used as-is).

## Reporting Issues

Use GitHub Issues to report bugs or request features. Include the following information:

- Steps to reproduce the problem
- Expected behavior vs. actual behavior
- Operating system and version
- Qt version (`qmake --version` or `pkg-config --modversion Qt5Core`)

## Architecture Reference

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a detailed overview of the system design and component relationships.
