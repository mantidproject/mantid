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

//
////=========================================================================================
//class MDCentroidPeaksTestPerformance : public CxxTest::TestSuite
//{
//public:
//  size_t numPeaks;
//  PeaksWorkspace_sptr peakWS;
//
//  // This pair of boilerplate methods prevent the suite being created statically
//  // This means the constructor isn't called when running other tests
//  static MDCentroidPeaksTestPerformance *createSuite() { return new MDCentroidPeaksTestPerformance(); }
//  static void destroySuite( MDCentroidPeaksTestPerformance *suite ) { delete suite; }
//
//
//  MDCentroidPeaksTestPerformance()
//  {
//    numPeaks = 1000;
//    // Original MDEW.
//    MDCentroidPeaksTest::createMDEW();
//
//    // Add a uniform, random background.
//    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
//        "InputWorkspace", "MDCentroidPeaksTest_MDEWS", "UniformParams", "100000");
//
//
//    // Make a fake instrument - doesn't matter, we won't use it really
//    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);
//
//    boost::mt19937 rng;
//    boost::uniform_real<double> u(-9.0, 9.0); // Random from -9 to 9.0
//    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > gen(rng, u);
//
//    peakWS = PeaksWorkspace_sptr(new PeaksWorkspace());
//    for (size_t i=0; i < numPeaks; ++i)
//    {
//      // Random peak center
//      double x = gen();
//      double y = gen();
//      double z = gen();
//
//      // Make the peak
//      MDCentroidPeaksTest::addPeak(1000, x,y,z, 0.02);
//      // With a center with higher density. 2000 events total.
//      MDCentroidPeaksTest::addPeak(1000, x,y,z, 0.005);
//
//      // Make a few very strong peaks
//      if (i%21 == 0)
//        MDCentroidPeaksTest::addPeak(10000, x,y,z, 0.015);
//
//      // Add to peaks workspace
//      peakWS->addPeak( Peak(inst, 1, 1.0, V3D(x, y, z) ) );
//
//      if (i%100==0)
//        std::cout << "Peak " << i << " added\n";
//    }
//    AnalysisDataService::Instance().add("MDCentroidPeaksTest_peaks",peakWS);
//
//  }
//
//  ~MDCentroidPeaksTestPerformance()
//  {
//    AnalysisDataService::Instance().remove("MDCentroidPeaksTest_MDEWS");
//    AnalysisDataService::Instance().remove("MDCentroidPeaksTest_peaks");
//  }
//
//
//  void setUp()
//  {
//
//  }
//
//  void tearDown()
//  {
//  }
//
//
//  void test_performance_NoBackground()
//  {
//    for (size_t i=0; i<10; i++)
//    {
//      MDCentroidPeaksTest::doRun(0.02, 0.0);
//    }
//    // All peaks should be at least 1000 counts (some might be more if they overla)
//    for (size_t i=0; i<numPeaks; i += 7)
//    {
//      double expected=2000.0;
//      if ((i % 21) == 0)
//          expected += 10000.0;
//      TS_ASSERT_LESS_THAN(expected-1, peakWS->getPeak(int(i)).getIntensity());
//    }
//  }
//
//  void test_performance_WithBackground()
//  {
//    for (size_t i=0; i<10; i++)
//    {
//      MDCentroidPeaksTest::doRun(0.02, 0.03);
//    }
//  }
//
//};


#endif /* MANTID_MDEVENTS_MDCENTROIDPEAKSTEST_H_ */

