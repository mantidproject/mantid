// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "../../../ISISReflectometry/GUI/Batch/RowPreprocessingAlgorithm.h"
#include "../../../ISISReflectometry/Reduction/IBatch.h"
#include "../../../ISISReflectometry/Reduction/PreviewRow.h"
#include "../../../ISISReflectometry/TestHelpers/ModelCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MockBatch.h"

#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;

class RowPreprocessingAlgorithmTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RowPreprocessingAlgorithmTest *createSuite() { return new RowPreprocessingAlgorithmTest(); }
  static void destroySuite(RowPreprocessingAlgorithmTest *suite) { delete suite; }

  void setUp() override { Mantid::API::FrameworkManager::Instance(); }

  void test_input_run_list_forwarded() {
    auto batch = MockBatch();
    auto expected = std::vector<std::string>{"12345"};
    auto row = PreviewRow(expected);
    auto alg = createConfiguredAlgorithm(batch, row);
    TS_ASSERT_EQUALS(expected, static_cast<decltype(expected)>(alg->algorithm()->getProperty("InputRunList")));
  }
};
