# Mantid Repository Analysis for Copilot Instructions

## 1. Repository Overview

- **Language Stack**: C++20 and Python 3.11
- **GUI Framework**: Qt5-based Workbench
- **Build System**: CMake 3.21+ with Ninja generator
- **Package Manager**: Conda (with pixi for development)
- **Purpose**: High-performance framework for materials-science data processing (Neutron scattering, Muon spectroscopy)
- **License**: GPL-3.0+
- **Key Links**:
  - Repository: https://github.com/mantidproject/mantid
  - Developer Site: https://developer.mantidproject.org

## 2. Project Structure

### Framework Directory (`/Framework`)

The core C++ library containing:

- **Kernel** - Low-level utilities (logging, properties, units, exceptions)
- **API** - High-level interfaces (Algorithms, Workspaces, FrameworkManager)
- **Algorithms** - 200+ algorithm implementations (data processing)
- **DataObjects** - Workspace implementations (Workspace2D, EventWorkspace, TableWorkspace, etc.)
- **DataHandling** - File I/O operations (Load, Save algorithms)
- **Geometry** - Instrument definitions and geometric calculations
- **Nexus/NexusGeometry** - HDF5-based data file handling
- **CurveFitting** - Fitting algorithms (peaks, functions)
- **MDAlgorithms** - Multi-dimensional data algorithms
- **Muon** - Muon-specific algorithms
- **Catalog** - Catalog system for managing data
- **Crystal** - Crystal-specific algorithms
- **LiveData** - Real-time data stream handling
- **Indexing** - Index-based data access
- **Types** - Type definitions
- **HistogramData** - Histogram data structures
- **Beamline** - Beamline-specific functionality
- **Reflectometry** - Reflectometry reduction algorithms
- **SINQ** - SINQ instrument support
- **Json** - JSON handling
- **Parallel** - MPI support

### Qt Directory (`/qt`)

GUI components for Workbench:

- **applications/** - Main Workbench application
- **python/** - MantidQt Python bindings
- **widgets/** - UI widget library
- **scientific_interfaces/** - Domain-specific interfaces (SANS, Reflectometry, etc.)
- **icons/** - Application icons and resources

### Scripts Directory (`/scripts`)

Python scripting and utilities:

- **Reduction facilities** (SANS, Inelastic, Engineering, Muon, etc.)
- **Calibration tools** (tube calibration, etc.)
- **AbINS module** (Inelastic neutron scattering analysis)
- **Reduction GUI** - User interface for reduction workflows
- **External interfaces** - Bridge to external tools (GSAS-II, etc.)

### Testing Directory (`/Testing`)

- **Tools/cxxtest/** - CxxTest framework files
- **Data/** - Test data and fixtures
- **SystemTests/** - System-level test scripts

### Instrument Directory (`/instrument`)

- **XML definition files** for 100+ instruments
- Facility definitions (ISIS, SNS, ESS, ILL, CSNS, etc.)
- Parameter files for specific instruments

### Dev Docs (`/dev-docs/source`)

Developer documentation including:

- Getting started guides
- IDE setup (PyCharm, VS Code, CLion)
- Build instructions
- Testing guides
- Design documents
- Git workflow
- Architecture documentation

## 3. Build Configuration

### CMakePresets.json

Available presets:

- **Development**: `linux`, `osx`, `win-vs`, `win-ninja` (Debug mode)
- **CI**: `linux-64-ci`, `osx-arm64-ci`, `win-64-ci` (Release mode)
- **Sanitizers**: `linux-64-ci-address-sanitiser`, `linux-64-ci-ub-sanitiser`, `linux-64-ci-thread-sanitiser`
- **Coverage**: `linux-64-ci-coverage`
- **Analysis**: `cppcheck-ci`, `doxygen-ci`

### Build Workflow

```bash
cmake --preset=linux . && cd build/
ninja Framework          # Build core framework
ninja workbench          # Build GUI application
ninja AllTests          # Build all tests
ctest                   # Run tests
./systemtest            # Run system tests
```

### buildconfig/CMake

Custom CMake modules:

- `FindCxxTest.cmake` - CxxTest integration
- `CommonSetup.cmake` - Common configuration
- `Bootstrap.cmake` - Dependency bootstrapping
- `Sanitizers.cmake` - Sanitizer support
- `CppCheck.cmake` - Static analysis
- Platform-specific: `LinuxSetup.cmake`, `WindowsSetup.cmake`, `DarwinSetup.cmake`

## 4. Code Formatting & Style

### Clang Format (`.clang-format`)

- **Base Style**: LLVM
- **Column Limit**: 120 characters
- **Language**: C++

### Clang Tidy (`.clang-tidy`)

Checks enabled:

- `cppcoreguidelines-*` (except array-related and init-variables)
- `modernize-*` (except trailing-return-type and type-traits)
- `misc-definitions-in-headers`, `misc-header-include-cycle`, `misc-include-cleaner`, `misc-misplaced-const`

### Pre-commit Hooks (`.pre-commit-config.yaml`)

- **Code formatters**: clang-format, mdformat, ruff-format, cmake-format, taplo
- **Linters**: ruff-check, rstcheck
- **Security**: gitleaks
- **File checks**: trailing-whitespace, check-merge-conflict, check-yaml, end-of-file-fixer
- **Custom**: cmake-missing-pytest-files, mantid-release-note-check, pixi-dependency-updater

### Python Code Style (`pyproject.toml`)

- **Tool**: Ruff (via pre-commit)
- **Line Length**: 140 characters
- **Key Rules**: E, F, W, FURB, RUF100, UP (upgrade), NPY201 (numpy 2), S (security)
- **Exceptions**: Bare except, asserts, xml modules allowed in certain contexts

## 5. C++ Code Conventions

### File Organization

- **Header files** (`.h`): Located in `inc/Mantid{ModuleName}/` subdirectories
- **Implementation** (`.cpp`): Located in `src/` subdirectories
- **Tests** (`.h`): Located in `test/` subdirectories, named `{ClassName}Test.h`

### File Naming

- **Classes**: PascalCase (e.g., `AddSampleLog.h`, `FFTSmooth.h`)
- **Files match class names** exactly
- **Test files**: `{AlgorithmName}Test.h` (e.g., `AddSampleLogTest.h`)

### Namespacing

Hierarchical namespace structure:

- **Root**: `Mantid`
- **Level 1**: `Mantid::API`, `Mantid::Kernel`, `Mantid::Algorithms`, `Mantid::DataObjects`, etc.
- **Nested**: `Mantid::Kernel::fft`, `Mantid::Kernel::spline`, `Mantid::API::AlgorithmProperties`

### Class Patterns

#### Algorithm Implementation

All algorithms inherit from `API::Algorithm`:

```cpp
namespace Mantid::Algorithms {

class MANTID_ALGORITHMS_DLL MyAlgorithm final : public API::Algorithm {
public:
  // Metadata methods (override)
  const std::string name() const override { return "MyAlgorithm"; }
  const std::string summary() const override { return "..."; }
  int version() const override { return 1; }
  const std::string category() const override { return "Category\\Subcategory"; }
  const std::vector<std::string> seeAlso() const override { return {...}; }

private:
  // Required overrides
  void init() override;    // Declare properties
  void exec() override;    // Execute algorithm

  // Helper methods (private)
  void helperMethod();
};

} // namespace Mantid::Algorithms

// In .cpp:
DECLARE_ALGORITHM(MyAlgorithm)  // Register with AlgorithmFactory
```

#### Declaration Macros

- `DECLARE_ALGORITHM(ClassName)` - Register algorithm in AlgorithmFactory
- `MANTID_ALGORITHMS_DLL` - Export symbol (per module, e.g., `MANTID_KERNEL_DLL`)

#### Property Declaration

In `init()` method:

```cpp
declareProperty("PropertyName", defaultValue, validator, "Description");
declareProperty(std::make_unique<WorkspaceProperty<T>>("WSName", "", Direction::Input), "Desc");
```

### Copyright Header

All files must include:

```cpp
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
```

## 6. Testing Structure

### C++ Unit Tests (CxxTest)

- **Framework**: CxxTest (header-based, code generation)
- **Location**: `{Module}/test/{ClassName}Test.h`
- **Pattern**: Classes inherit from `CxxTest::TestSuite`
- **Assertions**: `TS_ASSERT`, `TS_ASSERT_EQUALS`, `TS_ASSERT_DELTA`, `TS_ASSERT_THROWS`
- **Test Methods**: Prefix with `test` (e.g., `void testAddition()`)

Example:

```cpp
#pragma once
#include <cxxtest/TestSuite.h>

class MyTest : public CxxTest::TestSuite {
public:
  static MyTest *createSuite() { return new MyTest(); }
  static void destroySuite(MyTest *suite) { delete suite; }

  void test_basic_functionality() {
    TS_ASSERT_EQUALS(1 + 1, 2);
  }
};
```

### Python Unit Tests (unittest)

- **Framework**: unittest (standard library) or pytest (modern)
- **Naming**: Files named `*Test.py` or `test_*.py`
- **Pattern**: Classes inherit from `unittest.TestCase`
- **Methods**: Prefix with `test_`
- **Pytest config**: `pytest.ini` sets `python_files = *Test.py`

Example:

```python
import unittest


class MyTest(unittest.TestCase):
    def test_basic_functionality(self):
        self.assertEqual(1 + 1, 2)


if __name__ == "__main__":
    unittest.main()
```

### Running Tests

#### C++ Tests

```bash
cd build/
cmake --build . --target AllTests         # Build all tests
cmake --build . --target {ModuleTest}     # Build specific module tests
ctest -j8                                 # Run all tests parallel
ctest -R {TestName}                       # Run specific tests (regex)
./bin/{ModuleName}Test                    # Run directly
./bin/{ModuleName}Test TestClassName      # Run specific test class
./bin/{ModuleName}Test TestClassName testMethodName  # Run specific test
```

#### Python Tests

```bash
cd build/
# Via CMake/CTest
ctest -R python -j8

# Direct execution
python3 -m pytest /path/to/test_file.py::{TestClass}::{test_method}
python3 /path/to/test_file.py {TestClass}.{test_method}

# Run entire file
python3 /path/to/test_file.py
```

### Test Data

- **UnitTest data**: `/Testing/Data/UnitTest/`
- **SystemTest data**: `/Testing/Data/SystemTest/`
- **DocTest data**: `/Testing/Data/DocTest/`

### Test Helpers

- `MantidFrameworkTestHelpers` - C++ testing utilities
- `WorkspaceCreationHelper` - Create test workspaces
- `ComponentCreationHelper` - Create instrument components

## 7. Instrument XML Organization

### Structure

- **Root**: `/instrument/` contains XML definition files
- **Naming**: `{InstrumentName}_Definition[_{variant}].xml`
- **Parameter files**: `{InstrumentName}_Parameters[_{variant}].xml`
- **Facility info**: Defines source, choppers, sample positions, detector groups

### XML Content

- Instrument geometry (positions, distances)
- Detector mappings (spectrum to detector)
- Parameters (angles, distances, resolution functions)
- Monitor definitions
- Time-of-flight ranges

## 8. Python Module Organization

### Mantid Python Package Structure

```
scripts/
├── abins/                    # Inelastic neutron scattering analysis
├── Calibration/              # Calibration tools
├── Diffraction/              # Diffraction reduction
├── Engineering/              # Engineering instruments
├── Inelastic/                # Inelastic reduction
├── Muon/                      # Muon reduction
├── SANS/                      # SANS reduction
├── reduction/                # Common reduction framework
├── reduction_gui/            # Qt-based reduction GUI
├── Interface/                # User interfaces
├── ExternalInterfaces/       # External tool bridges
└── test/                      # Unit tests (mirror structure)
```

### Python Test Runner

- **Primary**: `unittest` (standard)
- **Modern**: `pytest` can also run tests
- **CMake integration**: `pyunittest_add_test()` macro registers tests

## 9. Key Design Patterns

### Algorithm Pattern

1. Create header in `inc/MantidAlgorithms/{AlgName}.h`
1. Create impl in `src/{AlgName}.cpp`
1. Create test in `test/{AlgName}Test.h`
1. Override `init()`, `exec()`, and metadata methods
1. Use `DECLARE_ALGORITHM(ClassName)` to register
1. Add to CMakeLists.txt file lists

### Workspace Pattern

- **MatrixWorkspace**: 2D histogram data (spectra × bins)
- **EventWorkspace**: Time-of-flight event data
- **TableWorkspace**: Column-based tabular data
- **MDWorkspace**: Multi-dimensional data
- **PeaksWorkspace**: Single-crystal peaks

### Property Pattern

- Use property validators for input checking
- Support multiple property types (numbers, strings, workspaces, arrays)
- Use `Direction::Input/Output/InOut` for workspace properties

## 10. Development Workflow

### Setup (from AGENTS.md)

```bash
pixi shell                    # Activate environment (or eval $(pixi shell-hook))
cmake --preset=linux . && cd build/
```

### Build

```bash
ninja Framework               # Core libraries only
ninja workbench              # Full GUI application
ninja AllTests               # All unit tests
```

### Testing

```bash
ctest                        # Run tests
./systemtest                 # System tests (in Testing/SystemTests/)
```

### Code Quality

```bash
clang-tidy -config=.clang-tidy ...  # Run clang-tidy
pre-commit run --all-files           # Run all pre-commit hooks
```

### PR Checklist

- Run clang-tidy (configuration in `.clang-tidy`)
- Run `pre-commit run --all-files` for formatting
- Follow PR template in `.github/PULL_REQUEST_TEMPLATE.md`
- Add release notes if applicable (docs/source/release/v\{version}/...)

## 11. Important Files & Configurations

| File                        | Purpose                                |
| --------------------------- | -------------------------------------- |
| `CMakeLists.txt`            | Top-level CMake configuration          |
| `CMakePresets.json`         | Build presets for different platforms  |
| `.clang-format`             | Code formatting (LLVM style, 120 cols) |
| `.clang-tidy`               | Static analysis configuration          |
| `.pre-commit-config.yaml`   | Git hooks (formatting, security, lint) |
| `pyproject.toml`            | Python project config (Ruff linter)    |
| `pixi.toml` / `pixi.lock`   | Pixi environment (Conda frontend)      |
| `buildconfig/CMake/*.cmake` | Custom CMake modules                   |
| `AGENTS.md`                 | Quick dev workflow guide               |

## 12. Documentation

### Developer Docs (`/dev-docs/source`)

- **Getting Started**: Setup guides by OS
- **Building**: CMake details, dependency management
- **Testing**: Unit test running, debugging
- **Architecture**: Design documents
- **Code Style**: Guidelines, examples
- **Tools**: IDE setup, profiling, debugging
- **Release Process**: Checklists, procedures

### Important Files

- `WritingAnAlgorithm.rst` - Complete algorithm development guide
- `RunningTheUnitTests.rst` - Test execution reference
- `UnitTestGoodPractice.rst` - Testing best practices
- `NewStarterC++.rst` / `NewStarterPython.rst` - Quick start guides

## 13. Key Dependencies & Tools

### Build

- CMake 3.21+
- Ninja (preferred)
- Visual Studio 2022 (Windows) or GCC/Clang (Linux/macOS)

### Runtime

- Python 3.11+
- Qt5 / PyQt5
- HDF5 (NeXus format)
- Boost
- Various scientific libraries (numpy, scipy, matplotlib via Conda)

### Development

- Git
- CxxTest (header-based)
- Clang-Format/Clang-Tidy
- Ruff (Python linter)
- Pre-commit (git hooks)

## 14. Notable Naming Conventions

### Algorithms

- PascalCase names (e.g., `AddSampleLog`, `DiffractionFocussing`)
- Descriptive action verbs
- Avoid ambiguity with standard operations

### Workspaces

- Also PascalCase (e.g., `MatrixWorkspace`, `EventWorkspace`)
- Indicate data structure type

### Properties

- Snake_case for naming (e.g., `InputWorkspace`, `OutputWorkspace`, `LogName`)
- Direction indicators: `Input`, `Output`, `InOut`
- Consistent naming across algorithms

### Files

- Match class name exactly (e.g., `class AddSampleLog` → `AddSampleLog.h`)
- Tests append `Test` (e.g., `AddSampleLogTest.h`)
- Directory structure matches namespace hierarchy
