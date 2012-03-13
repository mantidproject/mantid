#ifndef MANTID_CRYSTAL_PEAKINTENSITYVSRADIUSTEST_H_
#define MANTID_CRYSTAL_PEAKINTENSITYVSRADIUSTEST_H_

#include "MantidCrystal/PeakIntensityVsRadius.h"
#include "MantidDataObjects/Peak.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidAPI/FrameworkManager.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class PeakIntensityVsRadiusTest : public CxxTest::TestSuite
{
public:
  void test_Init()
  {
    PeakIntensityVsRadius alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW()
  {
    // ---- Start with empty MDEW ----
    FrameworkManager::Instance().exec("CreateMDWorkspace", 14,
        "Dimensions", "3",
        "Extents", "-10,10,-10,10,-10,10",
        "Names", "h,k,l",
        "Units", "-,-,-",
        "SplitInto", "5",
        "MaxRecursionDepth", "2",
        "OutputWorkspace", "PeakIntensityVsRadiusTest_MDEWS");
  }

  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeak(size_t num, double x, double y, double z, double radius)
  {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    FrameworkManager::Instance().exec("FakeMDEventData", 4,
        "InputWorkspace", "PeakIntensityVsRadiusTest_MDEWS", "PeakParams", mess.str().c_str());
  }

  void setUp()
  {
    // Fake MDWorkspace with 2 peaks
    createMDEW();
    addPeak(1000, 0, 0, 0, 1.0);
    addPeak(1000, 5, 5, 5, 1.0);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    Instrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5); // Unused fake instruement
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(0., 0., 0.) ) );
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(5., 5., 5.) ) );
    AnalysisDataService::Instance().addOrReplace("PeakIntensityVsRadiusTest_peaks",peakWS);
  }


  void test_exec()
  {
    // Name of the output workspace.
    std::string outWSName("PeakIntensityVsRadiusTest_OutputWS");
  
    PeakIntensityVsRadius alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "PeakIntensityVsRadiusTest_MDEWS") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", "PeakIntensityVsRadiusTest_peaks") );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("OutputWorkspace", outWSName) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CoordinatesToUse", "HKL") );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RadiusStart", 0.0) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("RadiusEnd", 1.5) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("NumSteps", 16) );
    TS_ASSERT_THROWS_NOTHING( alg.execute(); );
    TS_ASSERT( alg.isExecuted() );
    
    // Retrieve the workspace from data service.
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outWSName) );
    TS_ASSERT(ws); if (!ws) return;
    
    // Check the results
    TSM_ASSERT_EQUALS( "Two peaks", ws->getNumberHistograms(), 2);
    TSM_ASSERT_EQUALS( "16 radii specified", ws->blocksize(), 16);
    TS_ASSERT_DELTA( ws->dataX(0)[1], 0.1, 1e-6);
    TS_ASSERT_DELTA( ws->dataX(0)[2], 0.2, 1e-6);

    TS_ASSERT_LESS_THAN( ws->dataY(0)[5], 1000);
    TSM_ASSERT_DELTA( "After 1.0, the signal is flat", ws->dataY(0)[12], 1000, 1e-6);
    TSM_ASSERT_DELTA( "After 1.0, the signal is flat", ws->dataY(0)[15], 1000, 1e-6);

    
    // Remove workspace from the data service.
    AnalysisDataService::Instance().remove(outWSName);
  }


};


#endif /* MANTID_CRYSTAL_PEAKINTENSITYVSRADIUSTEST_H_ */
