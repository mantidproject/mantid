// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IMDNode.h"
#include "MantidAPI/IMDWorkspace.h"
#include "MantidDataObjects/MDHistoWorkspace.h"
#include "MantidFrameworkTestHelpers/MDEventsTestHelper.h"
#include "MantidMDAlgorithms/CloneMDWorkspace.h"
#include "MantidMDAlgorithms/CompareMDWorkspaces.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::DataObjects::MDEventsTestHelper;
using Mantid::DataObjects::MDHistoWorkspace_sptr;

class CompareMDWorkspacesTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    CompareMDWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void doTest(const std::string &ws1, const std::string &ws2, const std::string &resultExpected = "Success!",
              bool CheckEvents = true, bool IgnoreDifferentID = false) {

    CompareMDWorkspaces alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace1", ws1));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace2", ws2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CheckEvents", CheckEvents));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Tolerance", 1e-5));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IgnoreBoxID", IgnoreDifferentID));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    std::string result = alg.getPropertyValue("Result");
    std::cout << result << '\n';
    TSM_ASSERT(result.c_str(), result.starts_with(resultExpected));
  }

  void test_histo() {
    MDHistoWorkspace_sptr A = makeFakeMDHistoWorkspace(1.56, 3, 10, 10.0, 1.57, "A");
    MDHistoWorkspace_sptr B = makeFakeMDHistoWorkspace(1.56, 3, 10, 10.0, 1.57, "B");
    doTest("A", "B");
    B->setSignalAt(123, 2.34);
    doTest("A", "B", "MDHistoWorkspaces have a different signal at index 123");
    B->setSignalAt(123, 1.56);
    B->setErrorSquaredAt(123, 2.34);
    doTest("A", "B", "MDHistoWorkspaces have a different error at index 123");

    MDHistoWorkspace_sptr C = makeFakeMDHistoWorkspace(1.56, 3, 9, 10.0, 1.57, "C");
    doTest("A", "C", "Dimension #0 has a different number of bins");
    MDHistoWorkspace_sptr C2 = makeFakeMDHistoWorkspace(1.56, 3, 10, 20.0, 1.57, "C2");
    doTest("A", "C2", "Dimension #0 has a different maximum");

    MDHistoWorkspace_sptr D = makeFakeMDHistoWorkspace(1.56, 2, 10, 10.0, 1.57, "D");
    doTest("A", "D", "Workspaces have a different number of dimensions");
  }

  void test_md() {
    makeAnyMDEW<MDEvent<3>, 3>(2, 0., 10., 1, "A");
    makeAnyMDEW<MDLeanEvent<3>, 3>(2, 0., 10., 1, "mdle3");
    doTest("A", "mdle3", "Workspaces are of different types");

    makeAnyMDEW<MDEvent<3>, 3>(2, 0., 10., 2, "B");
    doTest("A", "B", "Box signal does not match");

    makeAnyMDEW<MDEvent<3>, 3>(3, 0., 10., 1, "C");
    doTest("A", "C", "Workspaces do not have the same number of boxes");

    CloneMDWorkspace cloner;
    cloner.initialize();
    cloner.setPropertyValue("InputWorkspace", "A");
    cloner.setPropertyValue("OutputWorkspace", "A1");
    TS_ASSERT_THROWS_NOTHING(cloner.execute());
    if (!cloner.isExecuted())
      return;

    doTest("A", "A1");

    auto mdWorkspace = dynamic_cast<IMDEventWorkspace *>(FrameworkManager::Instance().getWorkspace("A1"));
    TSM_ASSERT("Can not retrieve MD workspace A1 from analysis data service", mdWorkspace);

    std::vector<IMDNode *> boxes;
    mdWorkspace->getBoxes(boxes, 1000, false);

    boxes[0]->setID(10000);

    doTest("A", "A1", "Boxes have different ID (0 vs 10000)");

    // with ignore box ID the comparison is true
    doTest("A", "A1", "Success!", true, true);
  }
};
