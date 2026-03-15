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
- **Interfaces**: Prefixed with `I` (e.g., `IPriceHandler`, `IPlugin`)
- **Logging**: Use `Logger::info()`, `Logger::warn()`, and `Logger::error()` instead of raw `std::cout` or `qDebug()`

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
- Follow the existing patterns in `test_price_checker.cpp` and `test_repository.cpp`.
- Repository tests use in-memory SQLite (`:memory:`) to avoid filesystem dependencies.
- Add new test targets in `CMakeLists.txt` following the existing test target structure.

## Reporting Issues

Use GitHub Issues to report bugs or request features. Include the following information:

- Steps to reproduce the problem
- Expected behavior vs. actual behavior
- Operating system and version
- Qt version (`qmake --version` or `pkg-config --modversion Qt5Core`)

## Architecture Reference

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for a detailed overview of the system design and component relationships.
