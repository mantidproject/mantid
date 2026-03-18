# Mantid Code Patterns & Examples

## Algorithm Implementation Template

### Header File (`Framework/Algorithms/inc/MantidAlgorithms/MyAlgorithm.h`)

```cpp
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidAPI/MatrixWorkspace_fwd.h"
#include "MantidAlgorithms/DllConfig.h"

namespace Mantid::Algorithms {

/**
 * Brief description of algorithm
 * Longer description explaining what it does, parameters, and usage.
 *
 * @author Author Name
 * @date YYYY-MM-DD
 */
class MANTID_ALGORITHMS_DLL MyAlgorithm final : public API::Algorithm {
public:
  /// Algorithm's name (used for registration)
  const std::string name() const override { return "MyAlgorithm"; }

  /// Summary of algorithm's purpose
  const std::string summary() const override {
    return "Describes what the algorithm does in one sentence.";
  }

  /// Algorithm's version for identification
  int version() const override { return 1; }

  /// Algorithm's category (use backslash for hierarchy)
  const std::string category() const override {
    return "Transforms\\Smoothing";
  }

  /// See also links to related algorithms
  const std::vector<std::string> seeAlso() const override {
    return {"RelatedAlgo1", "RelatedAlgo2"};
  }

private:
  /// Initialize algorithm properties
  void init() override;

  /// Execute the algorithm
  void exec() override;

  /// Helper methods (private)
  void validateInputs();
  double calculateResult(const API::MatrixWorkspace_const_sptr &ws);
};

} // namespace Mantid::Algorithms
```

### Implementation File (`Framework/Algorithms/src/MyAlgorithm.cpp`)

```cpp
// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright © 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX-License-Identifier: GPL-3.0+
#include "MantidAlgorithms/MyAlgorithm.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ListValidator.h"
#include "MantidKernel/MandatoryValidator.h"

namespace Mantid::Algorithms {

// Register the algorithm into the AlgorithmFactory
DECLARE_ALGORITHM(MyAlgorithm)

using namespace Kernel;
using namespace API;

void MyAlgorithm::init() {
  // Input workspace (required)
  declareProperty(
    std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "InputWorkspace", "", Direction::Input),
    "Input workspace to process");

  // Output workspace (required)
  declareProperty(
    std::make_unique<WorkspaceProperty<MatrixWorkspace>>(
      "OutputWorkspace", "", Direction::Output),
    "Output workspace");

  // String property with validation
  std::vector<std::string> options{"Option1", "Option2", "Option3"};
  declareProperty("Method", "Option1",
    std::make_shared<StringListValidator>(options),
    "Choose processing method");

  // Numeric property
  declareProperty("Parameter", 1.0,
    std::make_shared<MandatoryValidator<double>>(),
    "Important parameter (must be provided)");

  // Optional property
  declareProperty("OptionalParam", 0.0,
    "Optional parameter (has default value)");
}

void MyAlgorithm::exec() {
  // Get input properties
  MatrixWorkspace_const_sptr inputWS = getProperty("InputWorkspace");
  std::string method = getProperty("Method");
  double parameter = getProperty("Parameter");

  // Validate inputs
  validateInputs();

  // Clone or create output workspace
  MatrixWorkspace_sptr outputWS = inputWS->clone();

  // Process data
  const size_t numHistograms = outputWS->getNumberHistograms();
  Progress progress(this, 0.0, 1.0, numHistograms);

  PARALLEL_FOR_IF(threadSafe(*outputWS))
  for (int i = 0; i < static_cast<int>(numHistograms); ++i) {
    PARALLEL_START_INTERRUPT_REGION

    auto &y = outputWS->mutableY(i);
    // Perform calculation
    for (size_t j = 0; j < y.size(); ++j) {
      y[j] = calculateResult(inputWS);
    }

    progress.report();

    PARALLEL_END_INTERRUPT_REGION
  }
  PARALLEL_CHECK_INTERRUPT_REGION

  // Set output
  setProperty("OutputWorkspace", outputWS);
}

void MyAlgorithm::validateInputs() {
  MatrixWorkspace_const_sptr ws = getProperty("InputWorkspace");
  if (!ws) {
    throw std::invalid_argument("InputWorkspace is required");
  }
}

double MyAlgorithm::calculateResult(const API::MatrixWorkspace_const_sptr &ws) {
  return 0.0; // Placeholder
}

} // namespace Mantid::Algorithms
```

### Test File (`Framework/Algorithms/test/MyAlgorithmTest.h`)

```cpp
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MyAlgorithm.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class MyAlgorithmTest : public CxxTest::TestSuite {
public:
  // Prevent suite creation before setup
  static MyAlgorithmTest *createSuite() {
    return new MyAlgorithmTest();
  }
  static void destroySuite(MyAlgorithmTest *suite) {
    delete suite;
  }

  void test_init() {
    MyAlgorithm alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
  }

  void test_basic_execution() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    MyAlgorithm alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("Method", "Option1");
    alg.setProperty("Parameter", 1.5);
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr output = alg.getProperty("OutputWorkspace");
    TS_ASSERT(output);
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 10);
  }

  void test_invalid_parameter() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    MyAlgorithm alg;
    alg.initialize();
    TS_ASSERT_THROWS(
      alg.setProperty("Method", "InvalidOption"),
      const std::invalid_argument &);
  }
};
```

## Python Algorithm Implementation

### Python Algorithm Template

```python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright © 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+

from mantid.api import PythonAlgorithm, AlgorithmFactory, MatrixWorkspaceProperty
from mantid.kernel import Direction, FloatBoundedValidator, StringListValidator
from mantid import logger


class MyPyAlgorithm(PythonAlgorithm):
    """
    Algorithm docstring explaining functionality.
    """

    def category(self):
        """Returns category for algorithm."""
        return "DataHandling\\Files"

    def seeAlso(self):
        """Returns list of related algorithms."""
        return ["Algorithm1", "Algorithm2"]

    def name(self):
        """Returns algorithm name."""
        return "MyPyAlgorithm"

    def version(self):
        """Returns algorithm version."""
        return 1

    def summary(self):
        """Returns brief description."""
        return "Does something useful with workspaces."

    def PyInit(self):
        """Initialize algorithm properties."""
        # Input workspace
        self.declareProperty(MatrixWorkspaceProperty("InputWorkspace", "", Direction.Input), "Input workspace to process")

        # Output workspace
        self.declareProperty(MatrixWorkspaceProperty("OutputWorkspace", "", Direction.Output), "Output workspace")

        # Numeric property with validator
        validator = FloatBoundedValidator(lower=0.0, upper=100.0)
        self.declareProperty("Parameter", 50.0, validator, "Parameter value (0-100)")

        # String property with fixed options
        self.declareProperty("Method", "Method1", StringListValidator(["Method1", "Method2", "Method3"]), "Processing method")

    def PyExec(self):
        """Execute the algorithm."""
        input_ws = self.getProperty("InputWorkspace").value
        parameter = self.getProperty("Parameter").value
        method = self.getProperty("Method").value

        # Validate inputs
        if not input_ws:
            raise ValueError("InputWorkspace is required")

        # Clone input workspace
        output_ws = input_ws.clone()

        # Process data
        logger.information(f"Processing with method: {method}")

        # Set output
        self.setProperty("OutputWorkspace", output_ws)
        logger.information("Algorithm completed successfully")


AlgorithmFactory.subscribe(MyPyAlgorithm)
```

### Python Test Template

```python
# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright © 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX-License-Identifier: GPL-3.0+

import unittest
from mantid.api import AnalysisDataService, AlgorithmFactory
from mantid.simpleapi import CreateSampleWorkspace, MyPyAlgorithm, DeleteWorkspaces


class MyPyAlgorithmTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        """Set up once for all tests."""
        pass

    def setUp(self):
        """Set up for each test."""
        self.test_ws = CreateSampleWorkspace()
        AnalysisDataService.addOrReplace("test_ws", self.test_ws)

    def tearDown(self):
        """Clean up after each test."""
        AnalysisDataService.clear()

    def test_basic_execution(self):
        """Test basic algorithm execution."""
        output = MyPyAlgorithm(InputWorkspace=self.test_ws, Parameter=50.0, Method="Method1")
        self.assertIsNotNone(output)

    def test_invalid_parameter(self):
        """Test that invalid parameter raises error."""
        with self.assertRaises(ValueError):
            MyPyAlgorithm(
                InputWorkspace=self.test_ws,
                Parameter=150.0,  # Out of bounds
            )


if __name__ == "__main__":
    unittest.main()
```

## CMakeLists.txt Pattern (Framework Module)

```cmake
# Framework/Algorithms/CMakeLists.txt

set(SRC_FILES
    src/Algorithm1.cpp
    src/Algorithm2.cpp
    src/MyAlgorithm.cpp
)

set(INC_FILES
    inc/MantidAlgorithms/Algorithm1.h
    inc/MantidAlgorithms/Algorithm2.h
    inc/MantidAlgorithms/MyAlgorithm.h
)

set(TEST_FILES
    test/Algorithm1Test.h
    test/Algorithm2Test.h
    test/MyAlgorithmTest.h
)

# Create the library
add_library(MantidAlgorithms ${SRC_FILES} ${INC_FILES})

# Link dependencies
target_link_libraries(
    MantidAlgorithms
    PUBLIC
        MantidAPI
        MantidKernel
    PRIVATE
        MantidDataObjects
)

# Install headers
install(
    FILES ${INC_FILES}
    DESTINATION include/mantid/algorithms
)

# Register unit tests
cxxtest_add_test(
    AlgorithmsTest
    ${CMAKE_CURRENT_BINARY_DIR}/AlgorithmsTestAutogen.cpp
    ${TEST_FILES}
)
target_link_libraries(AlgorithmsTest PRIVATE MantidAlgorithms)
```

## Key Patterns & Best Practices

### 1. Property Handling

```cpp
// Get input properties
auto input = getProperty("InputWorkspace");
auto param = getProperty("NumericParam");
std::string method = getProperty("StringParam");

// Check optional property
if (isDefault("OptionalParam")) {
  // Use default
}
```

### 2. Error Handling

```cpp
if (!condition) {
  throw std::invalid_argument("Descriptive error message");
}

try {
  // Algorithm code
} catch (const std::exception &e) {
  logger.error(e.what());
  throw;
}
```

### 3. Logging

```cpp
#include "MantidKernel/Logger.h"

auto &logger = Kernel::Logger::get("MyAlgorithm");
logger.debug("Debug message");
logger.information("Info message");
logger.warning("Warning message");
logger.error("Error message");
```

### 4. Progress Reporting

```cpp
Progress progress(this, 0.0, 1.0, numSteps);

for (size_t i = 0; i < numSteps; ++i) {
  // Do work
  progress.report();
}
```

### 5. Parallel Processing

```cpp
PARALLEL_FOR_IF(Kernel::threadSafe(*workspace))
for (int64_t i = 0; i < numHistograms; ++i) {
  PARALLEL_START_INTERRUPT_REGION

  // Process spectrum i

  PARALLEL_END_INTERRUPT_REGION
}
PARALLEL_CHECK_INTERRUPT_REGION
```

### 6. Workspace Cloning

```cpp
// Shallow clone (shared data)
MatrixWorkspace_sptr output = input->clone();

// Deep clone (separate data)
MatrixWorkspace_sptr output = input->cloneEmpty();
auto &outX = output->mutableX(i);
auto &outY = output->mutableY(i);
```

### 7. Dynamic Casting

```cpp
auto mws = std::dynamic_pointer_cast<MatrixWorkspace>(workspace);
if (mws) {
  // Use as MatrixWorkspace
}

auto ews = std::dynamic_pointer_cast<EventWorkspace>(workspace);
if (ews) {
  // Use as EventWorkspace
}
```

## File Organization Checklist

When adding a new algorithm:

1. **Create header**: `Framework/{Module}/inc/Mantid{Module}/{AlgName}.h`
1. **Create implementation**: `Framework/{Module}/src/{AlgName}.cpp`
1. **Create test**: `Framework/{Module}/test/{AlgName}Test.h`
1. **Add to CMakeLists**: Include in SRC_FILES, INC_FILES, and TEST_FILES lists
1. **Add copyright**: Include GPL-3.0+ header in all files
1. **Document**: Add docstring explaining algorithm purpose
1. **Register**: Use `DECLARE_ALGORITHM(ClassName)` in .cpp
1. **Test**: Write comprehensive test suite
1. **Format**: Run `clang-format` before commit
1. **Lint**: Run `clang-tidy` and `pre-commit` hooks
