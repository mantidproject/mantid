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

  void tearDown() override { AnalysisDataService::Instance().clear(); }

  void testGetType() {
    PolSANSWorkspaceValidator validator;
    TS_ASSERT_EQUALS(validator.getType(), "polSANS");
  }

  void testWorkspaceGroupWithThreeWorkspaces() {
    auto wsGroupWithThree = WorkspaceCreationHelper::createWorkspaceGroup(3, 1, 10, "test_ws");
    checkForErrorMessage(wsGroupWithThree, "The number of periods within the input workspace is not an allowed value.");
  }

  void testWorkspaceGroupWithThreeWorkspacesButItIsAllowed() {
    auto wsGroupWithThree = WorkspaceCreationHelper::createWorkspaceGroup(3, 1, 10, "test_ws");
    PolSANSWorkspaceValidator validator(true, false, {3});
    const auto result = validator.isValid(wsGroupWithThree);
    const auto pos = result.find("The number of periods within the input workspace is not an allowed value.");
    TS_ASSERT(pos == std::string::npos);
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
    auto wsGroupTOF = createWorkspaceGroup(1, "TOF");
    checkForErrorMessage(wsGroupTOF, "All workspaces must be in units of Wavelength.");
  }

  void testMultipleHistograms() {
    auto wsGroupMultipleHistograms = createWorkspaceGroup(2);
    checkForErrorMessage(wsGroupMultipleHistograms, "All workspaces must contain a single histogram.");
  }

  void testMultipleHistogramsWithAllowMultiPeriodActive() {
    auto wsGroupMultipleHistograms = createWorkspaceGroup(2);
    PolSANSWorkspaceValidator validator(true, true);
    const auto result = validator.isValid(wsGroupMultipleHistograms);
    const auto pos = result.find("All workspaces must contain a single histogram.");
    TS_ASSERT(pos == std::string::npos);
  }

  void testNonHistogramData() {
    auto wsGroupNonHistogram = createWorkspaceGroup(1, "Wavelength", false);
    checkForErrorMessage(wsGroupNonHistogram, "All workspaces must be histogram data.");
  }

  void testHistogramDataWithExpectHistoDataFalse() {
    auto wsGroupHistogram = createWorkspaceGroup();
    PolSANSWorkspaceValidator validator(false);
    const auto result = validator.isValid(wsGroupHistogram);
    const auto pos = result.find("All workspaces must not be histogram data.");
    TS_ASSERT(pos != std::string::npos);
  }

  void testWithExpectedData() {
    auto wsGroup = createWorkspaceGroup();
    PolSANSWorkspaceValidator validator;
    const auto result = validator.isValid(wsGroup);
    TS_ASSERT_EQUALS(result, "");
  }

private:
  void checkForErrorMessage(const WorkspaceGroup_sptr &ws, const std::string &message) {
    PolSANSWorkspaceValidator validator;
    const auto result = validator.isValid(ws);
    const auto pos = result.find(message);
    TS_ASSERT(pos != std::string::npos);
  }

  const WorkspaceGroup_sptr createWorkspaceGroup(const size_t nHist = 1, const std::string unit = "Wavelength",
                                                 const bool isHist = true) {
    auto wsGroup = std::make_shared<WorkspaceGroup>();
    for (int i = 0; i < 4; i++) {
      const auto ws = WorkspaceCreationHelper::create2DWorkspace123(nHist, 10, isHist);
      ws->getAxis(0)->setUnit(unit);
      wsGroup->addWorkspace(ws);
    }
    return wsGroup;
  }
};
