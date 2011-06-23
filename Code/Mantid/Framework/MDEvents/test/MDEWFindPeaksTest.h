#ifndef MANTID_MDEVENTS_MDEWFINDPEAKSTEST_H_
#define MANTID_MDEVENTS_MDEWFINDPEAKSTEST_H_

#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEWFindPeaks.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid::MDEvents;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using Mantid::Geometry::IInstrument_sptr;

class MDEWFindPeaksTest : public CxxTest::TestSuite
{
public:

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW()
  {
    // ---- Start with empty MDEW ----
    AlgorithmHelper::runAlgorithm("CreateMDEventWorkspace", 16,
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "h,k,l",
        "Units", "-,-,-",
        "SplitInto", "5",
        "SplitThreshold", "20",
        "MaxRecursionDepth", "15",
        "OutputWorkspace", "MDEWS");

    // Give it an instrument
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentRectangular2(1, 16);
    IMDEventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<IMDEventWorkspace>(AnalysisDataService::Instance().retrieve("MDEWS")) );
    ws->setInstrument(inst);

  }

  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeak(size_t num, double x, double y, double z, double radius)
  {
    std::ostringstream mess;
    mess << num/2 << ", " << x << ", " << y << ", " << z << ", " << radius;
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", "MDEWS", "PeakParams", mess.str().c_str());

    // Add a center with more events (half radius, half the total), to create a "peak"
    std::ostringstream mess2;
    mess2 << num/2 << ", " << x << ", " << y << ", " << z << ", " << radius/2;
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", "MDEWS", "PeakParams", mess2.str().c_str());
  }

    
  void test_Init()
  {
    MDEWFindPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  
  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("peaksFound");

    // Make the fake data
    createMDEW();
    addPeak(100, 1,2,3, 0.1);
    addPeak(300, 4,5,6, 0.2);
    addPeak(500, -5,-5,5, 0.2);
    // This peak will be rejected as non-physical
    addPeak(500, -5,-5,-5, 0.2);
  
    MDEWFindPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "MDEWS") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("DensityThresholdFactor", "2.0") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeakDistanceThreshold", "0.7") );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    PeaksWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<PeaksWorkspace>(AnalysisDataService::Instance().retrieve(outWSName)) );
    TS_ASSERT(ws);
    if (!ws) return;
    
    // Should find 3 peaks.
    TS_ASSERT_EQUALS( ws->getNumberPeaks(), 3);
    if (ws->getNumberPeaks() != 3) return;

    // The order of the peaks found is a little random because it depends on the way the boxes were sorted...
    TS_ASSERT_DELTA( ws->getPeak(0).getQLabFrame()[0], -5.0, 0.1);
    TS_ASSERT_DELTA( ws->getPeak(0).getQLabFrame()[1], -5.0, 0.1);
    TS_ASSERT_DELTA( ws->getPeak(0).getQLabFrame()[2],  5.0, 0.1);

    TS_ASSERT_DELTA( ws->getPeak(1).getQLabFrame()[0], 4.0, 0.1);
    TS_ASSERT_DELTA( ws->getPeak(1).getQLabFrame()[1], 5.0, 0.1);
    TS_ASSERT_DELTA( ws->getPeak(1).getQLabFrame()[2], 6.0, 0.1);

    TS_ASSERT_DELTA( ws->getPeak(2).getQLabFrame()[0], 1.0, 0.1);
    TS_ASSERT_DELTA( ws->getPeak(2).getQLabFrame()[1], 2.0, 0.1);
    TS_ASSERT_DELTA( ws->getPeak(2).getQLabFrame()[2], 3.0, 0.1);
    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }
  

};


#endif /* MANTID_MDEVENTS_MDEWFINDPEAKSTEST_H_ */

