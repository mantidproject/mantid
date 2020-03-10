// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOADMUONNEXUS3TEST_H_
#define LOADMUONNEXUS3TEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexus3.h"
#include "MantidDataObjects/Workspace2D.h"

#include <cmath>
#include <iostream>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexus3Test : public CxxTest::TestSuite {
private:
  // helper methods
public:
  void testExec() {
    LoadMuonNexus3 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "emu00098564.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("EntryNumber", "1"); // This will load the first period

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(
        output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outWS"));
  }
  void testExecMultiPeriod1() {
    LoadMuonNexus3 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "emu00098564.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("EntryNumber", "0"); // This will load  all periods

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify the number of periods, it should be two - hence there should be
    // two workspaces
    WorkspaceGroup_sptr outGrp;
    TS_ASSERT_THROWS_NOTHING(
        outGrp = AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "outWS"));

    Workspace_sptr ws_1;
    Workspace_sptr ws_2;
    TS_ASSERT_THROWS_NOTHING(ws_1 = outGrp->getItem(0));
    TS_ASSERT_THROWS_NOTHING(ws_1 = outGrp->getItem(1));

    Workspace2D_sptr output2D_1 =
        boost::dynamic_pointer_cast<Workspace2D>(ws_1);

    Workspace2D_sptr output2D_2 =
        boost::dynamic_pointer_cast<Workspace2D>(ws_1);

    const Mantid::API::Run &run1 = output2D_1->run();
    int goodfrm1 = run1.getPropertyAsIntegerValue("goodfrm");
    TS_ASSERT_EQUALS(goodfrm1, 11523);

    const Mantid::API::Run &run2 = output2D_2->run();
    int goodfrm2 = run1.getPropertyAsIntegerValue("goodfrm");
    TS_ASSERT_EQUALS(goodfrm2, 11524);
  }
  void testExecMultiPeriod2() {
    LoadMuonNexus3 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "emu00098564.nxs");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("EntryNumber", "2"); // This will load the second period

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    // Test workspace data
    MatrixWorkspace_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outWS"));
    Workspace2D_sptr output2D;
    TS_ASSERT_THROWS_NOTHING(
        output2D = boost::dynamic_pointer_cast<Workspace2D>(output));

    const Mantid::API::Run &run = output2D->run();
    int goodfrm = run.getPropertyAsIntegerValue("goodfrm");
    TS_ASSERT_EQUALS(goodfrm, 11523);
  }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMuonNexus3TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    loader.initialize();
    loader.setPropertyValue("Filename", "emu00098564.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    loader.setPropertyValue("EntryNumber",
                            "1"); // This will load the first period
  }

  void tearDown() override { AnalysisDataService::Instance().remove("ws"); }

  void testDefaultLoad() { loader.execute(); }

private:
  LoadMuonNexus3 loader;
};

#endif