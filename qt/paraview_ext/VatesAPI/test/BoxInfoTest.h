// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef VATES_API_BOX_INFO_TEST_H_
#define VATES_API_BOX_INFO_TEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/BoxController.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/MDLeanEvent.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidVatesAPI/ADSWorkspaceProvider.h"
#include "MantidVatesAPI/BoxInfo.h"

using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataObjects::MDEventsTestHelper;
using namespace Mantid::Kernel;

class BoxInfoTest : public CxxTest::TestSuite {
public:
  void test_initial_recursion_depth_is_empty_for_MD_Histo() {
    // Arrange
    const std::string wsName = "MD_HISTO_WS";
    auto ws = makeFakeMDHistoWorkspace(1.0, 4, 5, 1.0, 0.1, wsName);
    auto workspaceProvider = std::make_unique<
        Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>>();

    // Act + Assert
    TSM_ASSERT("Should have no recursion depth for top level splitting.",
               boost::none ==
                   Mantid::VATES::findRecursionDepthForTopLevelSplitting(
                       wsName, *workspaceProvider));

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_initial_recursion_depth_is_empty_for_MD_Event_wo_split() {
    // Arrange
    const std::string wsName = "MD_EVENT_WS";
    auto ws = makeAnyMDEW<MDLeanEvent<3>, 3>(10, 0.0, 10.0, 1, wsName);
    auto workspaceProvider = std::make_unique<
        Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>>();

    // Act + Assert
    TSM_ASSERT("Should have no recursion depth for top level splitting.",
               boost::none ==
                   Mantid::VATES::findRecursionDepthForTopLevelSplitting(
                       wsName, *workspaceProvider));

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }

  GNU_DIAG_OFF("strict-aliasing")
  void test_initial_recursion_depth_is_1_for_MD_Event_w_split() {
    // Arrange
    const std::string wsName = "MD_EVENT_WS_WITH_SPLITTING";
    auto ws = makeAnyMDEW<MDLeanEvent<3>, 3>(10, 0.0, 10.0, 1, wsName);
    BoxController_sptr boxController = ws->getBoxController();
    boxController->setSplitTopInto(0, 10);
    boxController->setSplitTopInto(1, 20);
    boxController->setSplitTopInto(2, 30);
    auto workspaceProvider = std::make_unique<
        Mantid::VATES::ADSWorkspaceProvider<Mantid::API::IMDEventWorkspace>>();

    // Act
    auto result = Mantid::VATES::findRecursionDepthForTopLevelSplitting(
        wsName, *workspaceProvider);
    // Assert

    TSM_ASSERT("Should have recursion depth of 1 for top level splitting.",
               result.get() == 1);

    // Clean up
    AnalysisDataService::Instance().remove(wsName);
  }
};

#endif
