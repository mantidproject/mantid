#ifndef SmoothNeighboursTEST_H_
#define SmoothNeighboursTEST_H_

#include "MantidGeometry/Instrument/INearestNeighboursFactory.h"
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


  void do_test(EventType type, double * expectedY, std::string WeightedSum = "Parabolic",  bool PreserveEvents = true,
      double Radius = 0.0,
      bool ConvertTo2D = false, int numberOfNeighbours=8)
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
    alg.setProperty("NumberOfNeighbours", numberOfNeighbours);
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

  /*
  * Test the Weighting strategies start.
  */

  void testNullWeightingStrategyAtRadiusThrows()
  {
    NullWeighting strategy;
    double distance = 0;
    TSM_ASSERT_THROWS("NullWeighting should always throw in usage", strategy.weightAt(distance), std::runtime_error);
  }

  void testNullWeightingStrategyRectangularThrows()
  {
    NullWeighting strategy;
    int adjX = 0;
    int adjY = 0;
    int ix = 0;
    int iy = 0;
    TSM_ASSERT_THROWS("NullWeighting should always throw in usage", strategy.weightAt(adjX, adjY, ix, iy), std::runtime_error);
  }

  void testFlatWeightingStrategyAtRadius()
  {
    FlatWeighting strategy;
    double distanceA = 0;
    double distanceB = 1000;
    TSM_ASSERT_EQUALS("FlatWeighting Should be distance insensitive", 1, strategy.weightAt(distanceA));
    TSM_ASSERT_EQUALS("FlatWeighting Should be distance insensitive", 1, strategy.weightAt(distanceB));
  }

  void testFlatWeightingStrategyRectangular()
  {
    FlatWeighting strategy;
    int adjX = 0;
    int adjY = 0;
    int ix = 0;
    int iy = 0;
    TSM_ASSERT_EQUALS("FlatWeighting Should be 1", 1, strategy.weightAt(adjX, ix, adjY, iy));
  }

  void testLinearWeightingAtRadius()
  {
    double cutOff = 2;
    LinearWeighting strategy(cutOff);

    double distance = 0;
    TSM_ASSERT_EQUALS("LinearWeighting should give full weighting at origin", 1, strategy.weightAt(distance));
    distance = 1;
    TSM_ASSERT_EQUALS("LinearWeighting should give 0.5 weighting at 1/2 radius", 0.5, strategy.weightAt(distance));
    distance = cutOff; //2
    TSM_ASSERT_EQUALS("LinearWeighting should give zero weighting at cutoff", 0, strategy.weightAt(distance));
  }

  void testLinearWeightingRectangular()
  {
    double cutOff = 0; //Doesn't matter what the cut off is.
    LinearWeighting strategy(cutOff);

    int adjX = 2;
    int adjY = 2; 

    int ix = 2; int iy = 2;
    TSM_ASSERT_EQUALS("Top-Right not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = 2;
    TSM_ASSERT_EQUALS("Top-Left not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Right not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Left not calculated properly", 0, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 0; iy = 0;
    TSM_ASSERT_EQUALS("Center not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 1; iy = 1;
    TSM_ASSERT_EQUALS("Half radius not calculated properly", 0.5, strategy.weightAt(adjX, ix, adjY, iy));
  }

  void testParabolicWeightingThrows()
  {
    ParabolicWeighting strategy;
    double distance = 0;
    TSM_ASSERT_THROWS("Should not be able to use the ParabolicWeighting like this.", strategy.weightAt(distance), std::runtime_error);
  }

  void testParabolicWeightingRectangular()
  {
    ParabolicWeighting strategy;

    int adjX = 2;
    int adjY = 2; 

    int ix = 2; int iy = 2;
    TSM_ASSERT_EQUALS("Top-Right not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = 2;
    TSM_ASSERT_EQUALS("Top-Left not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Right not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = -2; iy = -2;
    TSM_ASSERT_EQUALS("Bottom-Left not calculated properly", 1, strategy.weightAt(adjX, ix, adjY, iy));
    ix = 0; iy = 0;
    TSM_ASSERT_EQUALS("Center not calculated properly", 5, strategy.weightAt(adjX, ix, adjY, iy));
  }

  /*
  * End test weighting strategies.
  */

  /*
  * Start test Radius Filter.
  */
  void testRadiusThrowsIfNegativeCutoff()
  {
    TS_ASSERT_THROWS(RadiusFilter(-1), std::invalid_argument);
  }

  void testRadiusFiltering()
  {
    SpectraDistanceMap input;
    input[0] = 1;
    input[1] = 2;
    input[3] = 3;

    RadiusFilter filter(2);
    SpectraDistanceMap product = filter.apply(input);

    TSM_ASSERT_EQUALS("Should have kept all but one of the inputs", 2, product.size());
    TS_ASSERT_EQUALS(1, input[0]);
    TS_ASSERT_EQUALS(2, input[1]);
  }

  /*
  * End test radius filter
  */

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
    alg.setProperty("WeightedSum", "Flat");
    alg.setProperty("NumberOfNeighbours", 8);
    alg.setProperty("IgnoreMaskedDetectors", true);
    alg.setProperty("Radius", 1.2);
    alg.setProperty("RadiusUnits", "NumberOfPixels");
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
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
    do_test(TOF, expectedY, "Parabolic");
  }


  void test_event()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(TOF, expectedY);
  }

  void test_event_no_WeightedSum()
  {
    double expectedY[9] = {2, 2, 2, 2.3333, 2.3333, 2.3333, 2, 2, 2};
    do_test(TOF, expectedY, "Flat",  true /*PreserveEvents*/);
  }

  void test_event_Radius_no_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, 3.0, 2, 2, 2, 2};
    do_test(TOF, expectedY, "Flat", true /*PreserveEvents*/, 0.009 /* Radius */);
  }

  void test_event_Radius_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, (2. + 4.*9)/10., 2, 2, 2, 2};
    do_test(TOF, expectedY, "Linear", true /*PreserveEvents*/, 0.009 /* Radius */);
  }

  void test_workspace2D()
  {
    double expectedY[9] = {2, 2, 2, 2.3636, 2.5454, 2.3636, 2, 2, 2};
    do_test(TOF, expectedY, "Parabolic", false /*PreserveEvents*/,
        0.0 /* Radius*/, true /*Convert2D*/);
  }

  void test_workspace2D_no_WeightedSum()
  {
    double expectedY[9] = {2, 2, 2, 2.3333, 2.3333, 2.3333, 2, 2, 2};
    do_test(TOF, expectedY, "Flat", false /*PreserveEvents*/, 
        0.0 /* Radius*/, true /*Convert2D*/);
  }

  void test_workspace2D_Radius_no_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, 3.0, 2, 2, 2, 2};
    do_test(TOF, expectedY, "Flat", false /*PreserveEvents*/, 0.009 /* Radius */,
        true /*Convert2D*/);
  }

  void test_workspace2D_Radius_WeightedSum()
  {
    // Note: something seems off in the nearest neighbour calc for this fake instrument. It only finds the neighbours in a column
    double expectedY[9] = {2, 2, 2, 2, (2. + 4.*9)/10., 2, 2, 2, 2};
    do_test(TOF, expectedY, "Linear", false /*PreserveEvents*/, 0.009 /* Radius */,
        true /*Convert2D*/);
  }


};

#endif /*SmoothNeighboursTEST_H_*/


