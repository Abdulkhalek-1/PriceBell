# Building PriceBell

## Prerequisites

- **CMake** 3.16 or later
- **C++17 compiler**: GCC 9+, Clang 10+, or MSVC 2019+
- **Qt5 modules**: Widgets, Sql, Network, Svg, LinguistTools

### Platform-specific package installation

**Ubuntu / Debian:**

```bash
sudo apt install cmake g++ qtbase5-dev libqt5sql5-sqlite libqt5svg5-dev qttools5-dev
```

**Fedora:**

```bash
sudo dnf install cmake gcc-c++ qt5-qtbase-devel qt5-qtsvg-devel qt5-qttools-devel
```

**Arch Linux:**

```bash
sudo pacman -S cmake qt5-base qt5-svg qt5-tools
```

**macOS (Homebrew):**

```bash
brew install cmake qt@5
```

**Windows:**

Install Qt5 via the [Qt Online Installer](https://www.qt.io/download-qt-installer) or through [vcpkg](https://vcpkg.io/):

```
vcpkg install qt5-base qt5-svg qt5-tools
```

## Build Steps

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
./PriceBell
```

On macOS, replace `make -j$(nproc)` with `make -j$(sysctl -n hw.logicalcpu)`.

On Windows with MSVC, use the CMake generator for your toolchain:

```
cmake -G "Visual Studio 16 2019" ..
cmake --build . --config Release
```

## CMake Details

The CMake configuration enables `AUTOMOC`, `AUTOUIC`, and `AUTORCC` automatically, so Qt meta-object compilation, UI form processing, and resource compilation require no manual steps.

After a successful build, two post-build actions are performed:

1. A `plugins/` directory is created next to the output binary (for runtime plugin loading).
2. The `assets/` directory is copied alongside the binary so that themes and resources are available at runtime.

## Running Tests

```bash
cd build
ctest --output-on-failure
```

There are two test targets:

- **test_price_checker** -- validates price and discount filter/matching logic.
- **test_repository** -- exercises the SQLite ProductRepository and AlertRepository using an in-memory database.
