# PriceBell Native Plugin — Starter Example

This is the minimum viable native plugin for PriceBell. It implements `IPlugin2`
and always returns a price of `9.99`. Replace the `fetchProduct()` body with your real logic.

## Prerequisites

- Qt5 (same version used to build PriceBell)
- CMake 3.16+
- A C++17 compiler

## Building

From the `examples/plugin-starter` directory:

```bash
mkdir build && cd build
cmake .. -DPRICEBELL_SOURCE_DIR=/path/to/PriceBell
cmake --build .
```

The compiled plugin (`.so` on Linux, `.dll` on Windows) will appear in `build/examples/plugins/`.

Alternatively, build from the PriceBell repo root with the `BUILD_EXAMPLES` option:

```bash
cmake -B build-examples -DBUILD_EXAMPLES=ON
cmake --build build-examples --parallel $(nproc)
ls build-examples/examples/plugins/
```

## Installing

Copy the compiled `.so`/`.dll` to your PriceBell plugins directory
(set in Settings → Plugins → Plugin Directory).

## How it works

| Method | Purpose |
|--------|---------|
| `fetchProduct(url)` | Fetch the price. Return a `FetchResult` with `success=true`, `price`, and `discount`. |
| `handlerId()` | Return a unique string identifier for your plugin (e.g. `"my-shop"`). |
| `displayName()` | Human-readable name shown in the UI source selector. |
| `metadata()` | Return JSON with `id`, `name`, `version`, and `urlPatterns`. |
| `settingsWidget()` | Optional: return a `QWidget*` to embed in PriceBell's Settings → Plugins tab. |
| `icon()` | Optional: return a `QIcon` to display in PriceBell's plugin list. |

See [docs/PLUGINS.md](../../docs/PLUGINS.md) for the full plugin development guide.
