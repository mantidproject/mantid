// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidDataHandling/LoadMuonNexusV2.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/UnitFactory.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <cxxtest/TestSuite.h>

#include <cmath>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadMuonNexusV2Test : public CxxTest::TestSuite {
private:
  // helper methods
public:
  void testExec() {
    LoadMuonNexusV2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename", "EMU00103638.nxs_v2");
    ld.setPropertyValue("OutputWorkspace", "outWS");
    ld.setPropertyValue("DeadTimeTable", "DeadTimeTable");
    ld.setPropertyValue("DetectorGroupingTable", "DetectorGroupingTable");

    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());
    // Verify that the output workspace exists
    MatrixWorkspace_sptr output_ws;
    TS_ASSERT_THROWS_NOTHING(
        output_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outWS"));

    Workspace2D_sptr output2D =
        std::dynamic_pointer_cast<Workspace2D>(output_ws);

    const Mantid::API::Run &run = output2D->run();
    int goodfrm = run.getPropertyAsIntegerValue("goodfrm");
    TS_ASSERT_EQUALS(goodfrm, 36197);
    double firstGoodData = ld.getProperty("FirstGoodData");
    TS_ASSERT_EQUALS(firstGoodData, 0.384);
    double timeZero = ld.getProperty("TimeZero");
    TS_ASSERT_DELTA(timeZero, 0.1599999, 1e-5);

    // Check the unit has been set correctly
    TS_ASSERT_EQUALS(output2D->getAxis(0)->unit()->unitID(), "Label");
    TS_ASSERT(!output2D->isDistribution());

    // Check that sample temp and field set
    double temperature = run.getPropertyAsSingleValue("sample_temp");
    TS_ASSERT_EQUALS(100.0, temperature);
    double field = run.getPropertyAsSingleValue("sample_magn_field");
    TS_ASSERT_EQUALS(0.0, field);
  }

  void testExecWithGroupingTable() {}
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadMuonNexusV2TestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    loader.initialize();
    loader.setPropertyValue("Filename", "EMU00103638.nxs_v2");
    loader.setPropertyValue("OutputWorkspace", "ws");
  }

  void tearDown() override { AnalysisDataService::Instance().remove("ws"); }

  void testDefaultLoad() { loader.execute(); }

private:
  LoadMuonNexusV2 loader;
};
