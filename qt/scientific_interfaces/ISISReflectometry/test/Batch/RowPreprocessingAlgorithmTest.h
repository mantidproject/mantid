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
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MockBatch.h"

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

using namespace ::testing;

using namespace MantidQt::CustomInterfaces::ISISReflectometry;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::PreprocessRow;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::IConfiguredAlgorithm;

class RowPreprocessingAlgorithmTest : public CxxTest::TestSuite {
  class StubbedPreProcess : public WorkspaceCreationHelper::StubAlgorithm {
  public:
    StubbedPreProcess() {
      this->setChild(true);
      auto prop = std::make_unique<Mantid::API::WorkspaceProperty<>>(m_propName, "", Mantid::Kernel::Direction::Output);
      declareProperty(std::move(prop));
    }

    void addOutputWorkspace(Mantid::API::MatrixWorkspace_sptr &ws) {
      this->getPointerToProperty("OutputWorkspace")->createTemporaryValue();
      setProperty(m_propName, ws);
    }
    const std::string m_propName = "OutputWorkspace";
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RowPreprocessingAlgorithmTest *createSuite() { return new RowPreprocessingAlgorithmTest(); }
  static void destroySuite(RowPreprocessingAlgorithmTest *suite) { delete suite; }

  void test_input_run_list_forwarded() {
    auto batch = MockBatch();
    auto inputRuns = std::vector<std::string>{"12345"};
    auto row = PreviewRow(inputRuns);
    auto mockAlg = std::make_shared<StubbedPreProcess>();

    auto configuredAlg = createConfiguredAlgorithm(batch, row, mockAlg);
    TS_ASSERT_EQUALS(configuredAlg->algorithm(), mockAlg);
    auto expectedProps = IConfiguredAlgorithm::AlgorithmRuntimeProps{{"InputRunList", inputRuns[0]}};
    TS_ASSERT_EQUALS(configuredAlg->properties(), expectedProps);
  }

  void test_row_is_updated_on_algorithm_complete() {
    auto mockAlg = std::make_shared<StubbedPreProcess>();
    const bool isHistogram = true;
    Mantid::API::MatrixWorkspace_sptr mockWs = WorkspaceCreationHelper::create1DWorkspaceRand(1, isHistogram);
    mockAlg->addOutputWorkspace(mockWs);

    auto runNumbers = std::vector<std::string>{};
    auto row = PreviewRow(runNumbers);

    updateRowOnAlgorithmComplete(mockAlg, row);

    TS_ASSERT_EQUALS(row.getLoadedWs(), mockWs);
  }
};
