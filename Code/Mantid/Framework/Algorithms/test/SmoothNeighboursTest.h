#ifndef SmoothNeighboursTEST_H_
#define SmoothNeighboursTEST_H_

#include "MantidAlgorithms/SmoothNeighbours.h"
#include "MantidAlgorithms/CheckWorkspacesMatch.h"
#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <iostream>
#include <iomanip>

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataHandling/LoadInstrument.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Algorithms;

class SmoothNeighboursTest : public CxxTest::TestSuite
{
public:


  void do_test(EventType type, double * expectedY, bool PreserveEvents = true,
      bool WeightedSum = true, double Radius = 0.0,
      bool ConvertTo2D = false)
  {
    // Pixels will be spaced 0.008 apart
    EventWorkspace_sptr in_ws = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1, 20, false);

    if (type == WEIGHTED)
    {
      in_ws *= 2.0;
      in_ws *= 0.5;
    }
    if (type == WEIGHTED_NOTIME)
    {
      for (size_t i = 0; i<in_ws->getNumberHistograms(); i++)
      {
        EventList & el = in_ws->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }

    // Multiply by 2 the workspace at index 4
    EventList & el = in_ws->getEventList(4);
    el += el;

    size_t nevents0 = in_ws->getNumberEvents();

    AnalysisDataService::Instance().addOrReplace("SmoothNeighboursTest_input", in_ws);
    if (ConvertTo2D)
    {
      FrameworkManager::Instance().exec("ConvertToMatrixWorkspace", 4,
          "InputWorkspace", "SmoothNeighboursTest_input",
          "OutputWorkspace", "SmoothNeighboursTest_input");
    }


    // Register the workspace in the data service

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
    alg.setPropertyValue("InputWorkspace", "SmoothNeighboursTest_input");
    alg.setProperty("OutputWorkspace", "testEW");
    alg.setProperty("AdjX", 1);
    alg.setProperty("AdjY", 1);
    alg.setProperty("PreserveEvents", PreserveEvents);
    alg.setProperty("WeightedSum", WeightedSum);
    alg.setProperty("Radius", Radius);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    if (PreserveEvents)
    {
      EventWorkspace_sptr ws;
      TS_ASSERT_THROWS_NOTHING(
          ws = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("testEW")) );
      TS_ASSERT(ws);
      if (!ws) return;
      size_t nevents = ws->getNumberEvents();
      TS_ASSERT_LESS_THAN( nevents0, nevents);
    }

    // Check the values
    MatrixWorkspace_sptr ws = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testEW"));
    TS_ASSERT(ws);
    if (!ws) return;

    // Compare to expected values
    TS_ASSERT_DELTA(  ws->readY(0)[0], expectedY[0], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(1)[0], expectedY[1], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(2)[0], expectedY[2], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(3)[0], expectedY[3], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(4)[0], expectedY[4], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(5)[0], expectedY[5], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(6)[0], expectedY[6], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(7)[0], expectedY[7], 1e-4);
    TS_ASSERT_DELTA(  ws->readY(8)[0], expectedY[8], 1e-4);


    AnalysisDataService::Instance().remove("testEW");
  }

  void testWithUnsignedNumberOfNeighbours()
  {
    SmoothNeighbours alg;
    alg.initialize();
    TSM_ASSERT_THROWS("Cannot have number of neighbours < 1",alg.setProperty("NumberOfNeighbours", -1), std::invalid_argument);
  }

  void testWithNonIntegerNumberOfNeighbours()
  {
    SmoothNeighbours alg;
    alg.initialize();
    TSM_ASSERT_THROWS("Cannot have non-integer number of neighbours",alg.setProperty("NumberOfNeighbours", 1.1), std::invalid_argument);
  }

  void testWithValidNumberOfNeighbours()
  {
    SmoothNeighbours alg;
    alg.initialize();
    TSM_ASSERT_THROWS_NOTHING("A single neighbour is valid",alg.setProperty("NumberOfNeighbours", 1));
    int value = alg.getProperty("NumberOfNeighbours");
    TS_ASSERT_EQUALS(1, value);
  }

  void testWithNumberOfNeighbours()
  {
    MatrixWorkspace_sptr inWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(100, 10);

    SmoothNeighbours alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() );
    TS_ASSERT( alg.isInitialized() );
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", "testMW");
    alg.setProperty("PreserveEvents", false);
    alg.setProperty("WeightedSum", false);
    alg.setProperty("ProvideRadius", false);
    alg.setProperty("NumberOfNeighbours", 8);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );

    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(outWS = boost::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve("testMW")) );

    //Some basic checks
    TSM_ASSERT_EQUALS("Wrong number of histograms", inWS->getNumberHistograms(), outWS->getNumberHistograms());
    TSM_ASSERT_EQUALS("Wrong number of bins", inWS->readX(0).size(), outWS->readX(0).size());

    //Check that the workspaces are identical, including x and y values.
    CheckWorkspacesMatch* checkAlg = new CheckWorkspacesMatch;
    checkAlg->initialize();
    checkAlg->setProperty("Workspace1", inWS);
    checkAlg->setProperty("Workspace2", outWS);
    checkAlg->setProperty("Tolerance", 0.001);
    checkAlg->execute();
    std::string result = checkAlg->getProperty("Result");
    TS_ASSERT_EQUALS("Success!", result);

  }

  void test_event_WEIGHTED()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(WEIGHTED, expectedY);
  }

  void test_event_WEIGHTED_NOTIME()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(WEIGHTED_NOTIME, expectedY);
  }

  void test_event_dont_PreserveEvents()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(TOF, expectedY, false);
  }


  void test_event()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(TOF, expectedY);
  }

  void test_event_no_WeightedSum()
  {
    double expectedY[9] = {2, 2, 2, 2.3333, 2.3333, 2.3333, 2, 2, 2};
    do_test(TOF, expectedY, true /*PreserveEvents*/, false /*WeightedSum*/);
  }

  void test_event_Radius_no_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, 3.0, 2, 2, 2, 2};
    do_test(TOF, expectedY, true /*PreserveEvents*/, false /*WeightedSum*/, 0.009 /* Radius */);
  }

  void test_event_Radius_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, (2. + 4.*9)/10., 2, 2, 2, 2};
    do_test(TOF, expectedY, true /*PreserveEvents*/, true /*WeightedSum*/, 0.009 /* Radius */);
  }


  void test_workspace2D()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(TOF, expectedY, false /*PreserveEvents*/, true /*WeightedSum*/,
        0.0 /* Radius*/, true /*Convert2D*/);
  }

  void test_workspace2D_no_WeightedSum()
  {
    double expectedY[9] = {2, 2, 2, 2.3333, 2.3333, 2.3333, 2, 2, 2};
    do_test(TOF, expectedY, false /*PreserveEvents*/, false /*WeightedSum*/,
        0.0 /* Radius*/, true /*Convert2D*/);
  }

  void test_workspace2D_Radius_no_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, 3.0, 2, 2, 2, 2};
    do_test(TOF, expectedY, false /*PreserveEvents*/, false /*WeightedSum*/, 0.009 /* Radius */,
        true /*Convert2D*/);
  }

  void test_workspace2D_Radius_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, (2. + 4.*9)/10., 2, 2, 2, 2};
    do_test(TOF, expectedY, false /*PreserveEvents*/, true /*WeightedSum*/, 0.009 /* Radius */,
        true /*Convert2D*/);
  }


};

#endif /*SmoothNeighboursTEST_H_*/


