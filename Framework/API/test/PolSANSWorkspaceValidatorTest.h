// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/ITableWorkspace_fwd.h"
#include "MantidAPI/PolSANSWorkspaceValidator.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;

class PolSANSWorkspaceValidatorTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PolSANSWorkspaceValidatorTest *createSuite() { return new PolSANSWorkspaceValidatorTest(); }
  static void destroySuite(PolSANSWorkspaceValidatorTest *suite) { delete suite; }

  void tearDown() { AnalysisDataService::Instance().clear(); }

  void checkForErrorMessage(const WorkspaceGroup_sptr &ws, const std::string message) {
    PolSANSWorkspaceValidator validator;
    const auto result = validator.isValid(ws);
    const auto pos = result.find(message);
    TS_ASSERT(pos != std::string::npos);
  }

  void testGetType() {
    PolSANSWorkspaceValidator validator;
    TS_ASSERT_EQUALS(validator.getType(), "polSANS");
  }

  void testWorkspaceGroupWithThreeWorkspaces() {
    auto wsGroupWithThree = WorkspaceCreationHelper::createWorkspaceGroup(3, 1, 10, "test_ws");
    checkForErrorMessage(wsGroupWithThree, "Input workspace group must have 4 entries.");
  }

  void testTableWorkspaces() {
    auto wsGroupTabel = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = std::make_shared<TableWorkspace>();
      wsGroupTabel->addWorkspace(ws);
    }
    checkForErrorMessage(wsGroupTabel, "All workspaces must be of type MatrixWorkspace.");
  }

  void testUnitTOF() {
    auto wsGroupTOF = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 10, 0, 1);
      ws->getAxis(0)->setUnit("TOF");
      wsGroupTOF->addWorkspace(ws);
    }
    checkForErrorMessage(wsGroupTOF, "All workspaces must be in units of Wavelength.");
  }

  void testMultipleHistograms() {
    auto wsGroupMultipleHistograms = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(2, 10, 0, 1);
      wsGroupMultipleHistograms->addWorkspace(ws);
    }
    checkForErrorMessage(wsGroupMultipleHistograms, "All workspaces must contain a single histogram.");
  }

  void testMultipleHistogramsWithAllowMultiPeriodActive() {
    auto wsGroupMultipleHistograms = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspaceBinned(2, 10, 0, 1);
      wsGroupMultipleHistograms->addWorkspace(ws);
    }
    PolSANSWorkspaceValidator validator(true, true);
    const auto result = validator.isValid(wsGroupMultipleHistograms);
    const auto pos = result.find("All workspaces must contain a single histogram.");
    TS_ASSERT(pos == std::string::npos);
  }

  void testNonHistogramData() {
    auto wsGroupNonHistogram = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 10);
      wsGroupNonHistogram->addWorkspace(ws);
    }
    checkForErrorMessage(wsGroupNonHistogram, "All workspaces must be histogram data.");
  }

  void testHistogramDataWithExpectHistoDataFalse() {
    auto wsGroupHistogram = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 10, true);
      wsGroupHistogram->addWorkspace(ws);
    }
    PolSANSWorkspaceValidator validator(false);
    const auto result = validator.isValid(wsGroupHistogram);
    const auto pos = result.find("All workspaces must not be histogram data.");
    TS_ASSERT(pos != std::string::npos);
  }

  void testWithExpectedData() {
    auto wsGroup = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; ++i) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspace123(1, 10, true);
      ws->getAxis(0)->setUnit("Wavelength");
      wsGroup->addWorkspace(ws);
    }
    PolSANSWorkspaceValidator validator;
    const auto result = validator.isValid(wsGroup);
    TS_ASSERT_EQUALS(result, "");
  }
};
