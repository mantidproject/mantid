// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Axis.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceHistory.h"
#include "MantidAlgorithms/SofQW.h"
#include "MantidAlgorithms/SofQWNormalisedPolygon.h"
#include "MantidDataHandling/CreateSimulationWorkspace.h"
#include "MantidDataHandling/LoadNexusProcessed.h"
#include "MantidKernel/Unit.h"
#include <cxxtest/TestSuite.h>

#include <memory>

using namespace Mantid::API;

class SofQWCutTest : public CxxTest::TestSuite {
public:
  template <typename SQWType> WorkspaceGroup_sptr runSQW() {
    Mantid::DataHandling::LoadNexusProcessed loader;
    loader.initialize();
    loader.setChild(true);
    loader.setProperty("Filename", "MAR21335_Ei60meV.nxs");
    loader.setPropertyValue("OutputWorkspace", "__unused");
    loader.execute();

    Workspace_sptr loadedWS = loader.getProperty("OutputWorkspace");
    auto inWS = std::dynamic_pointer_cast<MatrixWorkspace>(loadedWS);

    // First make a cut along |Q|
    SQWType sqw_q;
    sqw_q.initialize();
    // Cannot be marked as child or history is not recorded
    TS_ASSERT_THROWS_NOTHING(sqw_q.setProperty("InputWorkspace", inWS));
    std::ostringstream wsname_q;
    wsname_q << "_tmp_q_" << loadedWS;
    TS_ASSERT_THROWS_NOTHING(sqw_q.setPropertyValue("OutputWorkspace", wsname_q.str()));
    TS_ASSERT_THROWS_NOTHING(sqw_q.setPropertyValue("QAxisBinning", "0,0.0125,10"));
    TS_ASSERT_THROWS_NOTHING(sqw_q.setPropertyValue("EMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(sqw_q.setPropertyValue("EAxisBinning", "-1.5,3,1.5"));
    TS_ASSERT_THROWS_NOTHING(sqw_q.execute());
    TS_ASSERT(sqw_q.isExecuted());

    // Now make a cut along E
    SQWType sqw_e;
    sqw_e.initialize();
    // Cannot be marked as child or history is not recorded
    TS_ASSERT_THROWS_NOTHING(sqw_e.setProperty("InputWorkspace", inWS));
    std::ostringstream wsname_e;
    wsname_e << "_tmp_e_" << loadedWS;
    TS_ASSERT_THROWS_NOTHING(sqw_e.setPropertyValue("OutputWorkspace", wsname_e.str()));
    TS_ASSERT_THROWS_NOTHING(sqw_e.setPropertyValue("QAxisBinning", "5,5,10"));
    TS_ASSERT_THROWS_NOTHING(sqw_e.setPropertyValue("EMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(sqw_e.setPropertyValue("EAxisBinning", "-5,0.5,55"));
    TS_ASSERT_THROWS_NOTHING(sqw_e.execute());
    TS_ASSERT(sqw_e.isExecuted());

    auto &dataStore = AnalysisDataService::Instance();
    auto ws_q = dataStore.retrieveWS<MatrixWorkspace>(wsname_q.str());
    auto ws_e = dataStore.retrieveWS<MatrixWorkspace>(wsname_e.str());
    WorkspaceGroup_sptr result = std::make_shared<WorkspaceGroup>();
    result->addWorkspace(ws_q);
    result->addWorkspace(ws_e);
    dataStore.remove(wsname_q.str());
    dataStore.remove(wsname_e.str());

    return result;
  }

  void test_sofqw3() {
    auto result = runSQW<Mantid::Algorithms::SofQWNormalisedPolygon>();
    const double delta(1e-08);

    auto ws_q = std::dynamic_pointer_cast<MatrixWorkspace>(result->getItem(0));
    TS_ASSERT_EQUALS(ws_q->getAxis(0)->length(), 2);
    TS_ASSERT_EQUALS(ws_q->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_EQUALS((*(ws_q->getAxis(0)))(0), -1.5);
    TS_ASSERT_EQUALS((*(ws_q->getAxis(0)))(1), 1.5);
    TS_ASSERT_EQUALS(ws_q->getAxis(1)->length(), 801);
    TS_ASSERT_EQUALS(ws_q->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS((*(ws_q->getAxis(1)))(0), 0.0);
    TS_ASSERT_DELTA((*(ws_q->getAxis(1)))(400), 5.0, delta);
    TS_ASSERT_EQUALS((*(ws_q->getAxis(1)))(800), 10.);
    TS_ASSERT_DELTA(ws_q->y(64)[0], 1.5577979780, delta);
    TS_ASSERT_DELTA(ws_q->e(64)[0], 0.0956304498, delta);
    TS_ASSERT_DELTA(ws_q->y(345)[0], 6.6485381486, delta);
    TS_ASSERT_DELTA(ws_q->e(345)[0], 0.2742620639, delta);
    TS_ASSERT_DELTA(ws_q->y(595)[0], 1.5959507353, delta);
    TS_ASSERT_DELTA(ws_q->e(595)[0], 0.1205590343, delta);
    TS_ASSERT_DELTA(ws_q->y(683)[0], 1.7956989694, delta);
    TS_ASSERT_DELTA(ws_q->e(683)[0], 0.1921823844, delta);
    TS_ASSERT_DELTA(ws_q->y(745)[0], 22.9332050574, delta);
    TS_ASSERT_DELTA(ws_q->e(745)[0], 0.5068253372, delta);

    auto ws_e = std::dynamic_pointer_cast<MatrixWorkspace>(result->getItem(1));
    TS_ASSERT_EQUALS(ws_e->getAxis(0)->length(), 121);
    TS_ASSERT_EQUALS(ws_e->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_EQUALS((*(ws_e->getAxis(0)))(0), -5.);
    TS_ASSERT_DELTA((*(ws_e->getAxis(0)))(60), 25.0, delta);
    TS_ASSERT_EQUALS((*(ws_e->getAxis(0)))(120), 55.);
    TS_ASSERT_EQUALS(ws_e->getAxis(1)->length(), 2);
    TS_ASSERT_EQUALS(ws_e->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_EQUALS((*(ws_e->getAxis(1)))(0), 5.);
    TS_ASSERT_EQUALS((*(ws_e->getAxis(1)))(1), 10.);
    TS_ASSERT_DELTA(ws_e->y(0)[3], 3.3384754287, delta);
    TS_ASSERT_DELTA(ws_e->e(0)[3], 0.0228727752, delta);
    TS_ASSERT_DELTA(ws_e->y(0)[20], 0.2282186682, delta);
    TS_ASSERT_DELTA(ws_e->e(0)[20], 0.0062779325, delta);
    TS_ASSERT_DELTA(ws_e->y(0)[27], 0.2638646427, delta);
    TS_ASSERT_DELTA(ws_e->e(0)[27], 0.0068527377, delta);
    TS_ASSERT_DELTA(ws_e->y(0)[78], 0.3287522505, delta);
    TS_ASSERT_DELTA(ws_e->e(0)[78], 0.0090771986, delta);
    TS_ASSERT_DELTA(ws_e->y(0)[119], 0.0453667614, delta);
    TS_ASSERT_DELTA(ws_e->e(0)[119], 0.0054619518, delta);
  }

  void test_sofqw3_zerobinwidth() {
    // This test sets up a workspace which can yield a bin with zero width
    // to check the code FractionalRebinning code handles this correctly
    const double delta(1e-08);
    Mantid::DataHandling::CreateSimulationWorkspace create_ws;
    create_ws.initialize();
    create_ws.setChild(true);
    create_ws.setPropertyValue("Instrument", "MARI");
    create_ws.setPropertyValue("BinParams", "-5,0.5,24");
    create_ws.setPropertyValue("OutputWorkspace", "__unused");
    create_ws.execute();

    MatrixWorkspace_sptr createdWS = create_ws.getProperty("OutputWorkspace");
    auto inWS = std::dynamic_pointer_cast<MatrixWorkspace>(createdWS);
    // Sets one spectrum to zero so final value is not unity.
    inWS->setCounts(300, std::vector<double>(58, 0.));

    Mantid::Algorithms::SofQWNormalisedPolygon sqw;
    sqw.initialize();
    sqw.setChild(true);
    TS_ASSERT_THROWS_NOTHING(sqw.setProperty("InputWorkspace", inWS));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("OutputWorkspace", "__unused"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("QAxisBinning", "0,10,10"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EMode", "Direct"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EFixed", "25"));
    TS_ASSERT_THROWS_NOTHING(sqw.setPropertyValue("EAxisBinning", "-1.5,3,1.5"));
    TS_ASSERT_THROWS_NOTHING(sqw.execute());
    TS_ASSERT(sqw.isExecuted());

    MatrixWorkspace_sptr ws = sqw.getProperty("OutputWorkspace");
    TS_ASSERT_EQUALS(ws->getAxis(0)->length(), 2);
    TS_ASSERT_EQUALS(ws->getAxis(0)->unit()->unitID(), "DeltaE");
    TS_ASSERT_EQUALS((*(ws->getAxis(0)))(0), -1.5);
    TS_ASSERT_EQUALS((*(ws->getAxis(0)))(1), 1.5);
    TS_ASSERT_EQUALS(ws->getAxis(1)->length(), 2);
    TS_ASSERT_EQUALS(ws->getAxis(1)->unit()->unitID(), "MomentumTransfer");
    TS_ASSERT_DELTA(ws->y(0)[0], 0.998910675, delta);
  }
};
