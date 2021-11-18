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
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/AlgorithmRuntimeProps.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"
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
      auto prop = std::make_unique<Mantid::API::WorkspaceProperty<Mantid::API::Workspace>>(
          m_propName, "", Mantid::Kernel::Direction::Output);
      declareProperty(std::move(prop));
    }

    void addOutputWorkspace(Mantid::API::Workspace_sptr &ws) {
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
    MantidQt::API::AlgorithmRuntimeProps expectedProps;
    expectedProps.setPropertyValue("InputRunList", inputRuns[0]);
    const auto &setProps = configuredAlg->getAlgorithmRuntimeProps();
    const auto &propNames = expectedProps.getDeclaredPropertyNames();

    TS_ASSERT(std::all_of(propNames.cbegin(), propNames.cend(), [&setProps, &expectedProps](const std::string &name) {
      return setProps.existsProperty(name) &&
             expectedProps.getPropertyValue(name) == expectedProps.getPropertyValue(name);
    }))
  }

  void test_row_is_updated_on_algorithm_complete() {
    auto mockAlg = std::make_shared<StubbedPreProcess>();
    const bool isHistogram = true;
    Mantid::API::Workspace_sptr mockWs = WorkspaceCreationHelper::create1DWorkspaceRand(1, isHistogram);
    mockAlg->addOutputWorkspace(mockWs);

    auto runNumbers = std::vector<std::string>{};
    auto row = PreviewRow(runNumbers);

    updateRowOnAlgorithmComplete(mockAlg, row);

    TS_ASSERT_EQUALS(row.getLoadedWs(), mockWs);
  }
};
