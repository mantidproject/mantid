#ifndef MANTID_MDEVENTS_MDCENTROIDPEAKSTEST_H_
#define MANTID_MDEVENTS_MDCENTROIDPEAKSTEST_H_

#include "MantidMDEvents/MDCentroidPeaks.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDCentroidPeaks.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/pow.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using Mantid::API::AnalysisDataService;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDEvents;


class MDCentroidPeaksTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    MDCentroidPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


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
        "BinarySplit", "0",
        "SplitInto", "5",
        "MaxRecursionDepth", "2",
        "OutputWorkspace", "MDCentroidPeaksTest_MDEWS");
  }


  //-------------------------------------------------------------------------------
  /** Add a fake "peak"*/
  static void addPeak(size_t num, double x, double y, double z, double radius)
  {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 6,
        "InputWorkspace", "MDCentroidPeaksTest_MDEWS",
        "PeakParams", mess.str().c_str(),
        "RandomSeed", "1234");

  }


  //-------------------------------------------------------------------------------
  /** Run the MDCentroidPeaks with the given peak radius param */
  static void doRun( V3D startHKL, double PeakRadius, V3D expectedHKL)
  {
    // Make a fake instrument - doesn't matter, we won't use it really
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->addPeak( Peak(inst, 1, 1.0, startHKL ) );
    TS_ASSERT_EQUALS( peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().addOrReplace("MDCentroidPeaksTest_Peaks", peakWS);

    MDCentroidPeaks alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "MDCentroidPeaksTest_MDEWS" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", "MDCentroidPeaksTest_Peaks" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CoordinatesToUse", "HKL" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeakRadius", PeakRadius ) );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );

    // Compare the result to the expectation
    TS_ASSERT_DELTA( peakWS->getPeak(0).getH(), expectedHKL[0], 0.05);
    TS_ASSERT_DELTA( peakWS->getPeak(0).getK(), expectedHKL[1], 0.05);
    TS_ASSERT_DELTA( peakWS->getPeak(0).getL(), expectedHKL[2], 0.05);

    AnalysisDataService::Instance().remove("MDCentroidPeaksTest_Peaks");
  }

  //-------------------------------------------------------------------------------
  /** Full test using faked-out peak data */
  void test_exec()
  {
    // --- Fake workspace with 3 peaks ------
    createMDEW();
    addPeak(1000, 0.,0.,0., 1.0);
    addPeak(1000, 2.,3.,4., 0.5);
    addPeak(1000, 6.,6.,6., 2.0);

    MDEventWorkspace3::sptr mdews = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve("MDCentroidPeaksTest_MDEWS"));
    TS_ASSERT_EQUALS( mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA( mdews->getBox()->getSignal(), 3000.0, 1e-2);

    // Start at the center, get the center
    doRun(V3D( 0.,0.,0.), 1.0, V3D( 0.,0.,0.));
    // Somewhat off center
    doRun(V3D( 0.2,0.2,0.2), 1.8, V3D( 0.,0.,0.));

    // Start at the center, get the center
    doRun(V3D( 2.,3.,4.), 1.0, V3D( 2.,3.,4.));

    // Pretty far off
    doRun(V3D( 1.5,2.5,3.5), 3.0, V3D( 2.,3.,4.));

    // Include two peaks, get the centroid of the two
    doRun(V3D( 1.0,1.5,2.0), 4.0, V3D( 1.0,1.5,2.0));

    // Include nothing, get no change
    doRun(V3D( 8.0, 0.0, 1.0), 1.0, V3D( 8.0, 0.0, 1.0));

    // Small radius still works
    doRun(V3D( 0.,0.,0.), 0.1, V3D( 0.,0.,0.));

    AnalysisDataService::Instance().remove("MDCentroidPeaksTest_MDEWS");
  }




};

#endif /* MANTID_MDEVENTS_MDCENTROIDPEAKSTEST_H_ */

