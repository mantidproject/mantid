// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once
#pragma once

#include "../../../ISISReflectometry/GUI/Batch/SumBanksAlgorithm.h"
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
using namespace MantidQt::CustomInterfaces::ISISReflectometry::SumBanks;
using namespace MantidQt::CustomInterfaces::ISISReflectometry::ModelCreationHelper;
using MantidQt::API::AlgorithmRuntimeProps;
using MantidQt::API::IConfiguredAlgorithm;

class SumBanksAlgorithmTest : public CxxTest::TestSuite {
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
  static SumBanksAlgorithmTest *createSuite() { return new SumBanksAlgorithmTest(); }
  static void destroySuite(SumBanksAlgorithmTest *suite) { delete suite; }

  void test_input_properties_forwarded() {
    auto batch = MockBatch();
    const bool isHistogram = true;
    Mantid::API::MatrixWorkspace_sptr mockWs = WorkspaceCreationHelper::create1DWorkspaceRand(1, isHistogram);
    auto detIDs = std::vector<Mantid::detid_t>{2, 3};
    auto row = PreviewRow({});
    row.setLoadedWs(mockWs);
    row.setSelectedBanks(detIDs);
    auto mockAlg = std::make_shared<StubbedPreProcess>();

    const std::string inputPropName = "InputWorkspace";
    const std::string roiPropName = "ROIDetectorIDs";
    auto configuredAlg = createConfiguredAlgorithm(batch, row, mockAlg);
    auto expectedProps = std::make_unique<AlgorithmRuntimeProps>();
    expectedProps->setProperty(inputPropName, mockWs);
    expectedProps->setPropertyValue(roiPropName, "2,3");
    const auto &setProps = configuredAlg->getAlgorithmRuntimeProps();

    TS_ASSERT_EQUALS(configuredAlg->algorithm(), mockAlg);
    TS_ASSERT_EQUALS(static_cast<decltype(mockWs)>(expectedProps->getProperty(inputPropName)),
                     static_cast<decltype(mockWs)>(setProps.getProperty(inputPropName)))
    TS_ASSERT_EQUALS(expectedProps->getPropertyValue(roiPropName), setProps.getPropertyValue(roiPropName))
  }

  void test_row_is_updated_on_algorithm_complete() {
    auto mockAlg = std::make_shared<StubbedPreProcess>();
    const bool isHistogram = true;
    Mantid::API::MatrixWorkspace_sptr mockWs = WorkspaceCreationHelper::create1DWorkspaceRand(1, isHistogram);
    mockAlg->addOutputWorkspace(mockWs);

    auto runNumbers = std::vector<std::string>{};
    auto row = PreviewRow(runNumbers);

    updateRowOnAlgorithmComplete(mockAlg, row);

    TS_ASSERT_EQUALS(row.getSummedWs(), mockWs);
  }
};
