# Mantid Copilot Instructions

Mantid is a C++20 / Python 3.11 framework for processing materials-science data (neutron scattering, muon spectroscopy). It has a Qt5 GUI frontend called Workbench. Build system is CMake + Ninja; dependencies are managed via Conda/Pixi.

## Environment setup

```bash
pixi shell                        # activate dev environment (or: eval $(pixi shell-hook))
cmake --preset=linux .            # configure (use win64 or osx-arm64 on other platforms)
cd build/
```

## Build

```bash
ninja Framework       # core C++ libraries only
ninja workbench       # full GUI application
ninja AllTests        # all unit test executables
```

## Testing

```bash
# Run all tests
ctest

# Run tests matching a pattern (regex against test name)
ctest -R SpecialWorkspace2D

# Run a specific C++ test executable directly
./bin/DataObjectsTest                                    # all tests in module
./bin/DataObjectsTest SpecialWorkspace2DTest             # one test class
./bin/DataObjectsTest SpecialWorkspace2DTest test_constructor_from_detids  # one method

# Run a Python test
python -m pytest path/to/MyTest.py::MyTest::test_method
python path/to/MyTest.py MyTest.test_method

# System tests
./systemtest
```

## Linting and formatting

```bash
pre-commit run --all-files        # format + lint everything (clang-format, ruff, cmake-format, etc.)
clang-tidy -config=.clang-tidy <file>
```

C++ style: LLVM base, **120-column limit** (`.clang-format`).
Python style: Ruff, **140-column limit** (`pyproject.toml`).

______________________________________________________________________

## Architecture

### Framework (`Framework/`)

The non-GUI C++ core. Each subdirectory is a separate shared library with its own `CMakeLists.txt`, `inc/Mantid{Module}/`, `src/`, and `test/` directories.

Dependency order (low → high):

```
Types → Kernel → Geometry / HistogramData → API → DataObjects → Algorithms / DataHandling / …
```

Key modules:

- **Kernel** – logging, properties, units, validators, thread pool
- **API** – `Algorithm`, `MatrixWorkspace`, `FrameworkManager`, factory singletons
- **DataObjects** – concrete workspace types (`Workspace2D`, `EventWorkspace`, `TableWorkspace`, `GroupingWorkspace`, …)
- **Algorithms** – 200+ data-processing algorithms
- **DataHandling** – file I/O (Load/Save algorithms for NeXus, ISIS raw, etc.)
- **Geometry** – instrument XML parsing, detector positions
- **CurveFitting** – peak and function fitting

### Qt GUI (`qt/`)

- `applications/` – Workbench entry point
- `widgets/` – shared UI components
- `scientific_interfaces/` – domain-specific reduction GUIs (SANS, Reflectometry, Muon, …)
- `python/` – Python bindings for Qt widgets

### Scripts (`scripts/`)

Pure-Python reduction workflows and GUIs, organized by technique (SANS, Inelastic, Muon, Engineering, Diffraction, …). Test files mirror the source structure under `scripts/test/`.

______________________________________________________________________

## Key conventions

### Algorithm pattern (C++)

Every algorithm inherits `API::Algorithm`, is registered with `DECLARE_ALGORITHM(ClassName)` in the `.cpp`, and overrides exactly these methods:

```cpp
// .h
class MANTID_ALGORITHMS_DLL MyAlg final : public API::Algorithm {
public:
  const std::string name() const override { return "MyAlg"; }
  const std::string summary() const override { return "One sentence."; }
  int version() const override { return 1; }
  const std::string category() const override { return "Category\\Sub"; }
private:
  void init() override;   // declare properties
  void exec() override;   // run algorithm
};

// .cpp
DECLARE_ALGORITHM(MyAlg)
```

Properties are declared in `init()` via `declareProperty(...)`. Workspace properties use `WorkspaceProperty<T>`. Parallel loops use the `PARALLEL_FOR_IF` / `PARALLEL_START_INTERRUPT_REGION` / `PARALLEL_CHECK_INTERRUPT_REGION` macros.

### Adding a new algorithm

1. Header: `Framework/{Module}/inc/Mantid{Module}/{Name}.h`
1. Impl: `Framework/{Module}/src/{Name}.cpp`
1. Test: `Framework/{Module}/test/{Name}Test.h`
1. Register all three file lists in the module's `CMakeLists.txt` (`SRC_FILES`, `INC_FILES`, `TEST_FILES`)
1. Add a `.rst` doc page under `docs/source/algorithms/`
1. Add a release note entry under `docs/source/release/v{version}/`

### C++ file layout

- Headers live in `inc/Mantid{Module}/` and use `#pragma once`
- Namespace: `namespace Mantid::{Module} { … }` (single-line C++17 style)
- Export macro per module: `MANTID_KERNEL_DLL`, `MANTID_API_DLL`, `MANTID_DATAOBJECTS_DLL`, etc.

### C++ tests (CxxTest)

Test files are `.h` files in `test/`, named `{Class}Test.h`. They inherit `CxxTest::TestSuite`. Test methods are prefixed `test_`. The CMake macro `cxxtest_add_test({Module}Test ${TEST_FILES})` compiles them into a single executable named `{Module}Test`.

```cpp
#pragma once
#include <cxxtest/TestSuite.h>
#include "MantidDataObjects/SpecialWorkspace2D.h"

class SpecialWorkspace2DTest : public CxxTest::TestSuite {
public:
  void test_something() {
    TS_ASSERT_EQUALS(1 + 1, 2);
    TS_ASSERT_THROWS_NOTHING(someCall());
    TS_ASSERT_DELTA(val, expected, 1e-6);
  }
};
```

Test helpers live in `MantidFrameworkTestHelpers`: `WorkspaceCreationHelper`, `ComponentCreationHelper`.

### Python tests

Files are named `*Test.py` or `test_*.py` and use `unittest.TestCase`. Pytest can also run them. CMake registers them with `pyunittest_add_test`.

### Copyright header

All files must start with:

```cpp
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
```

(Use `#` comments for Python.)

### Instrument XML

`instrument/` contains `{Name}_Definition[_{variant}].xml` and matching `{Name}_Parameters[_{variant}].xml` files describing detector geometry for 100+ instruments across ISIS, SNS, ESS, ILL, CSNS, and other facilities.

______________________________________________________________________

## Further reading

- `AGENTS.md` – concise dev workflow cheat sheet
- `dev-docs/source/` – full developer documentation (getting started, architecture, IDE setup, testing guide)
- `.github/copilot/codebase-analysis.md` – detailed module reference
- `.github/copilot/code-patterns.md` – complete code templates (algorithm, Python algorithm, CMakeLists)
