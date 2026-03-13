# Mantid Copilot Instructions - Reference Material

This directory contains comprehensive analysis and code patterns for the Mantid repository. These files provide everything needed to write effective `copilot-instructions.md`.

## Files in This Package

### 1. **mantid_analysis.md** (451 lines)

Complete structural reference for the Mantid codebase.

**Contents:**

- Repository overview (C++20, Python 3.11, Qt5 GUI, CMake, Conda)
- Full project structure (Framework modules, Qt GUI, scripts, testing, instruments)
- Build system (CMakePresets.json, build targets, workflow)
- Code formatting (clang-format, clang-tidy, pre-commit hooks)
- C++ conventions (file organization, naming, namespaces, algorithm pattern)
- Testing structure (CxxTest for C++, unittest for Python)
- Instrument XML organization
- Development workflow
- Important files and dependencies

**Best for:** Understanding overall structure, finding what goes where, referencing conventions

### 2. **mantid_code_patterns.md** (400+ lines)

Ready-to-use code templates and implementation patterns.

**Contents:**

- Complete C++ algorithm template (header, implementation, test)
- Python algorithm template (PyInit/PyExec pattern)
- CMakeLists.txt module pattern
- Key coding patterns (properties, error handling, logging, parallelism, workspaces)
- File organization checklist
- Copyright headers

**Best for:** Starting new code, learning by example, quick reference patterns

### 3. **COPILOT_INSTRUCTIONS_REFERENCE.md** (This file)

Index and navigation guide for the reference materials.

## Quick Navigation

### By Task

**Starting a new algorithm:**

1. Read: mantid_analysis.md § 5 (C++ Conventions)
1. Use: mantid_code_patterns.md § "Algorithm Implementation Template"
1. Checklist: mantid_code_patterns.md § "File Organization Checklist"

**Writing tests:**

1. Read: mantid_analysis.md § 6 (Testing Structure)
1. Use: mantid_code_patterns.md § "Test File Template"
1. Run: mantid_analysis.md § 9 (Running Tests)

**Understanding the build:**

1. Read: mantid_analysis.md § 3 (Build Configuration)
1. Reference: mantid_analysis.md § 4 (Code Formatting & Style)

**Code style and formatting:**

1. Read: mantid_analysis.md § 4 (Code Formatting & Style)
1. Reference: mantid_code_patterns.md § "Key Patterns & Best Practices"

**Working with Python:**

1. Read: mantid_analysis.md § 8 (Python Module Organization)
1. Use: mantid_code_patterns.md § "Python Algorithm Implementation"
1. Test: mantid_analysis.md § 6 (Python Unit Tests)

### By File Type

**Header Files (.h)**

- Location: `Framework/{Module}/inc/Mantid{Module}/`
- Template: mantid_code_patterns.md § "Header File"
- Conventions: mantid_analysis.md § 5 (C++ Conventions)

**Implementation Files (.cpp)**

- Location: `Framework/{Module}/src/`
- Template: mantid_code_patterns.md § "Implementation File"
- Pattern: Include DECLARE_ALGORITHM macro

**Test Files (TestName.h or test_name.py)**

- C++ Tests: `Framework/{Module}/test/`, CxxTest framework
- Python Tests: `scripts/test/`, unittest framework
- Templates: mantid_code_patterns.md § "Test Templates"
- Running: mantid_analysis.md § 9

**CMakeLists.txt**

- Pattern: mantid_code_patterns.md § "CMakeLists.txt Pattern"
- Reference: mantid_analysis.md § 3 (Build Configuration)

**Instrument XML Files**

- Location: `/instrument/`
- Format: `{InstrumentName}_Definition.xml`
- Details: mantid_analysis.md § 7

## Key Facts (Quick Reference)

| Aspect             | Details                                                  |
| ------------------ | -------------------------------------------------------- |
| **Language**       | C++20 (primary), Python 3.11, Qt5 (GUI)                  |
| **Build**          | CMake 3.21+, Ninja generator, Conda/Pixi for deps        |
| **Code Format**    | LLVM style (clang-format), 120 column limit              |
| **C++ Tests**      | CxxTest framework, header files (.h), CxxTest::TestSuite |
| **Python Tests**   | unittest, files named \*Test.py, unittest.TestCase       |
| **Class Naming**   | PascalCase (AddSampleLog, FFTSmooth)                     |
| **File Naming**    | Match class name exactly (AddSampleLog.h/cpp)            |
| **Properties**     | snake_case (InputWorkspace, LogName)                     |
| **Namespaces**     | Mantid::Module::Submodule (e.g., Mantid::Algorithms)     |
| **Registration**   | DECLARE_ALGORITHM(ClassName) in .cpp                     |
| **Tests Location** | `test/{ClassName}Test.h` or `test/test_{module}.py`      |
| **Linting**        | clang-tidy (C++), ruff (Python)                          |
| **Pre-commit**     | clang-format, ruff, cmake-format, mdformat + security    |
| **Docs**           | `/dev-docs/source/` (RST format)                         |

## Algorithm Development Workflow

```
1. Choose module (e.g., Algorithms)
2. Create header: Framework/Algorithms/inc/MantidAlgorithms/{AlgName}.h
3. Create implementation: Framework/Algorithms/src/{AlgName}.cpp
4. Create test: Framework/Algorithms/test/{AlgName}Test.h
5. Add to CMakeLists.txt (SRC_FILES, INC_FILES, TEST_FILES)
6. Use template from mantid_code_patterns.md
7. Register: DECLARE_ALGORITHM(ClassName) in .cpp
8. Build: ninja Framework && ninja AlgorithmsTest
9. Test: ./bin/AlgorithmsTest
10. Format: clang-format -i *.{h,cpp}
11. Lint: clang-tidy --checks=... {files}
12. Pre-commit: pre-commit run --all-files
```

## Framework Modules

**Core Libraries:**

- **Kernel** - Low-level utilities (properties, logging, exceptions)
- **API** - High-level interfaces (Algorithms, Workspaces)
- **Types** - Type definitions
- **DataObjects** - Workspace implementations

**Data Processing:**

- **Algorithms** - 200+ algorithm implementations
- **DataHandling** - File I/O (Load/Save algorithms)
- **CurveFitting** - Fitting algorithms
- **Geometry** - Geometric calculations
- **HistogramData** - Histogram structures

**Specialized:**

- **Nexus/NexusGeometry** - HDF5-based I/O
- **MDAlgorithms** - Multi-dimensional data
- **Muon** - Muon-specific algorithms
- **Reflectometry** - Reflectometry reduction
- **LiveData** - Real-time data streams
- **Catalog** - Data catalog system

See mantid_analysis.md § 2 for complete details.

## Common Property Types

```cpp
// Workspace property (most common)
declareProperty(std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
    "InputWorkspace", "", Direction::Input), "Description");

// String with fixed options
declareProperty("Method", "Option1",
    std::make_shared<StringListValidator>({"Option1", "Option2"}),
    "Description");

// Numeric with validation
declareProperty("Parameter", 1.0,
    std::make_shared<FloatBoundedValidator>(0.0, 100.0)),
    "Description");

// Required property
declareProperty("Required", "",
    std::make_shared<MandatoryValidator<std::string>>(),
    "Description");
```

## Testing Examples

### C++ Unit Test

```cpp
class MyTest : public CxxTest::TestSuite {
public:
    void test_basic() { TS_ASSERT_EQUALS(1+1, 2); }
    void test_algorithm() { TS_ASSERT_THROWS_NOTHING(...); }
};
```

### Python Unit Test

```python
class MyTest(unittest.TestCase):
    def test_basic(self):
        self.assertEqual(1 + 1, 2)

    def test_with_setup(self):
        ws = CreateSampleWorkspace()
        self.assertIsNotNone(ws)
```

## Debugging & Development

**Run specific C++ test:**

```bash
./bin/AlgorithmsTest MyTestClassName
./bin/AlgorithmsTest MyTestClassName myTestMethod
```

**Run specific Python test:**

```bash
python3 -m pytest test_module.py::TestClass::test_method
python3 test_module.py TestClass.test_method
```

**Build with debug symbols:**

```bash
cmake --preset=linux . && cd build/
ninja Framework
```

**Run with clang-tidy:**

```bash
clang-tidy -p build/compile_commands.json file.cpp
```

## Important Links

- **Repository:** https://github.com/mantidproject/mantid
- **Developer Site:** https://developer.mantidproject.org
- **Issue Tracking:** https://github.com/mantidproject/mantid/issues
- **Build Server:** https://builds.mantidproject.org
- **Developer Docs:** `/dev-docs/source/` (in repo)

## File Checklist for New Algorithms

- [ ] Header file created (inc/Mantid\{Module}/\{AlgName}.h)
- [ ] Implementation file created (src/\{AlgName}.cpp)
- [ ] Test file created (test/\{AlgName}Test.h)
- [ ] Added to CMakeLists.txt (SRC_FILES, INC_FILES, TEST_FILES)
- [ ] DECLARE_ALGORITHM macro in .cpp
- [ ] Metadata methods implemented (name, summary, version, category)
- [ ] Properties declared in init()
- [ ] Logic implemented in exec()
- [ ] Test class created and methods written
- [ ] Copyright headers added (GPL-3.0+)
- [ ] Code formatted (clang-format)
- [ ] Linted (clang-tidy)
- [ ] Pre-commit hooks pass
- [ ] Builds successfully (ninja \{Module}Test)
- [ ] Tests pass (ctest -R \{TestName})

## When Writing Copilot-Instructions.md

**Recommended structure:**

1. **Overview** - What Mantid is, tech stack, purpose
1. **Getting Started** - Setup, build, first build
1. **Project Structure** - Where code lives, module organization
1. **Development Workflow** - Edit → Build → Test → Format → Commit
1. **Code Style** - Formatting, naming, conventions
1. **Testing** - How to write and run tests
1. **Algorithm Development** - Complete walkthrough with templates
1. **Debugging** - Tools and techniques
1. **Common Tasks** - Adding property, testing, etc.
1. **Resources** - Links to dev-docs, issue tracker, examples

**Reference these files heavily:**

- Use exact paths from mantid_analysis.md
- Include templates from mantid_code_patterns.md
- Cross-reference sections for deep learning
- Point to dev-docs for extended topics

## How to Use These Files

1. **For quick answers:** Check the Key Facts table above
1. **For structure questions:** See mantid_analysis.md § 2 (Project Structure)
1. **For coding questions:** Use mantid_code_patterns.md templates
1. **For convention questions:** See mantid_analysis.md § 5 & 11 (Naming)
1. **For testing questions:** See mantid_analysis.md § 6 & mantid_code_patterns.md tests
1. **For build questions:** See mantid_analysis.md § 3 (Build Configuration)
1. **For complete context:** Read mantid_analysis.md top to bottom
1. **For implementation:** Copy-paste from mantid_code_patterns.md and customize

______________________________________________________________________

**Created:** 2024
**Mantid Version:** Latest (from main branch)
**Analysis Scope:** Framework, Qt GUI, Python scripts, Testing, Instruments
