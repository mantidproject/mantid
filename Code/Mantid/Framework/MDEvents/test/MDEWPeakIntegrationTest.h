#ifndef MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_
#define MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidGeometry/IInstrument.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidMDEvents/MDEWPeakIntegration.h"
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


class MDEWPeakIntegrationTest : public CxxTest::TestSuite
{
public:

  void test_Init()
  {
    MDEWPeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  

  //-------------------------------------------------------------------------------
  /** Run the MDEWPeakIntegration with the given peak radius integration param */
  static void doRun(double PeakRadius, double BackgroundRadius)
  {
    MDEWPeakIntegration alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("InputWorkspace", "MDEWPeakIntegrationTest_MDEWS" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("PeaksWorkspace", "MDEWPeakIntegrationTest_peaks" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setPropertyValue("CoordinatesToUse", "HKL" ) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("PeakRadius", PeakRadius ) );
    TS_ASSERT_THROWS_NOTHING( alg.setProperty("BackgroundRadius", BackgroundRadius ) );
    TS_ASSERT_THROWS_NOTHING( alg.execute() );
    TS_ASSERT( alg.isExecuted() );
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
        "OutputWorkspace", "MDEWPeakIntegrationTest_MDEWS");
  }


  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeak(size_t num, double x, double y, double z, double radius)
  {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", "MDEWPeakIntegrationTest_MDEWS", "PeakParams", mess.str().c_str());

  }


  //-------------------------------------------------------------------------------
  /** Full test using faked-out peak data */
  void test_exec()
  {
    // --- Fake workspace with 3 peaks ------
    createMDEW();
    addPeak(1000, 0.,0.,0., 1.0);
    addPeak(1000, 2.,3.,4., 0.5);
    addPeak(1000, 5.,5.,5., 2.0);

    MDEventWorkspace3::sptr mdews = boost::dynamic_pointer_cast<MDEventWorkspace3>(AnalysisDataService::Instance().retrieve("MDEWPeakIntegrationTest_MDEWS"));
    TS_ASSERT_EQUALS( mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA( mdews->getBox()->getSignal(), 3000.0, 1e-2);

    // Make a fake instrument - doesn't matter, we won't use it really
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(0., 0., 0.) ) );
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(2., 3., 4.) ) );
    peakWS->addPeak( Peak(inst, 1, 1.0, V3D(5., 5., 5.) ) );

    TS_ASSERT_EQUALS( peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("MDEWPeakIntegrationTest_peaks",peakWS);

    // ------------- Integrate with 1.0 radius ------------------------
    doRun(1.0,0.0);

    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    // Peak is of radius 2.0, but we get half that radius = ~1/8th the volume
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(),  125.0, 10);

    // Error is also calculated
    TS_ASSERT_DELTA( peakWS->getPeak(0).getSigmaIntensity(), sqrt(1000.0), 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getSigmaIntensity(), sqrt(1000.0), 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(2).getSigmaIntensity(), sqrt(peakWS->getPeak(2).getIntensity()), 1e-2);


    // ------------- Let's do it again with 2.0 radius ------------------------
    doRun(2.0,0.0);

    // All peaks are fully contained
    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(), 1000.0, 1e-2);

    // ------------- Let's do it again with 0.5 radius ------------------------
    doRun(0.5,0.0);

    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 125.0, 10);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(), 15.0, 10);

    // ===============================================================================
    // ---- Now add a background signal over one peak--------------
    addPeak(1000, 0.,0.,0., 2.0);

    // ------------- Integrate with 1.0 radius and 2.0 background------------------------
    doRun(1.0, 2.0);
    // Same 1000 since the background (~125) was subtracted, with some random variation of the BG around
    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1000.0, 10);
    // Error on peak is the SUM of the error of peak and the subtracted background
    TS_ASSERT_DELTA( peakWS->getPeak(0).getSigmaIntensity(), sqrt(1125.0 + 125.0), 2.0);

    // Had no bg, so they are the same
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(1).getSigmaIntensity(), sqrt(1000.0), 1e-1);

    // This one is a 2.0 radius fake peak, so the background and peak have ~ the same density! So ~0 total intensity.
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(), 0.0, 12);
    // But the error is large since it is 125 - 125 (with errors)
    TS_ASSERT_DELTA( peakWS->getPeak(2).getSigmaIntensity(), sqrt(2*125.0), 2.);


    // ------------- Integrating without the background gives higher counts ------------------------
    doRun(1.0, 0.0);

    // +125 counts due to background
    TS_ASSERT_DELTA( peakWS->getPeak(0).getIntensity(), 1125.0, 10);

    // These had no bg, so they are the same
    TS_ASSERT_DELTA( peakWS->getPeak(1).getIntensity(), 1000.0, 1e-2);
    TS_ASSERT_DELTA( peakWS->getPeak(2).getIntensity(), 125.0, 10);

    AnalysisDataService::Instance().remove("MDEWPeakIntegrationTest_MDEWS");
    AnalysisDataService::Instance().remove("MDEWPeakIntegrationTest_peaks");
  }






};


//=========================================================================================
class MDEWPeakIntegrationTestPerformance : public CxxTest::TestSuite
{
public:
  size_t numPeaks;
  PeaksWorkspace_sptr peakWS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MDEWPeakIntegrationTestPerformance *createSuite() { return new MDEWPeakIntegrationTestPerformance(); }
  static void destroySuite( MDEWPeakIntegrationTestPerformance *suite ) { delete suite; }


  MDEWPeakIntegrationTestPerformance()
  {
    numPeaks = 1000;
    // Original MDEW.
    MDEWPeakIntegrationTest::createMDEW();

    // Add a uniform, random background.
    AlgorithmHelper::runAlgorithm("FakeMDEventData", 4,
        "InputWorkspace", "MDEWPeakIntegrationTest_MDEWS", "UniformParams", "100000");


    // Make a fake instrument - doesn't matter, we won't use it really
    IInstrument_sptr inst = ComponentCreationHelper::createTestInstrumentCylindrical(5);

    boost::mt19937 rng;
    boost::uniform_real<double> u(-9.0, 9.0); // Random from -9 to 9.0
    boost::variate_generator<boost::mt19937&, boost::uniform_real<double> > gen(rng, u);

    peakWS = PeaksWorkspace_sptr(new PeaksWorkspace());
    for (size_t i=0; i < numPeaks; ++i)
    {
      // Random peak center
      double x = gen();
      double y = gen();
      double z = gen();

      // Make the peak
      MDEWPeakIntegrationTest::addPeak(1000, x,y,z, 0.02);
      // With a center with higher density. 2000 events total.
      MDEWPeakIntegrationTest::addPeak(1000, x,y,z, 0.005);

      // Make a few very strong peaks
      if (i%21 == 0)
        MDEWPeakIntegrationTest::addPeak(10000, x,y,z, 0.015);

      // Add to peaks workspace
      peakWS->addPeak( Peak(inst, 1, 1.0, V3D(x, y, z) ) );

      if (i%100==0)
        std::cout << "Peak " << i << " added\n";
    }
    AnalysisDataService::Instance().add("MDEWPeakIntegrationTest_peaks",peakWS);

  }

  ~MDEWPeakIntegrationTestPerformance()
  {
    AnalysisDataService::Instance().remove("MDEWPeakIntegrationTest_MDEWS");
    AnalysisDataService::Instance().remove("MDEWPeakIntegrationTest_peaks");
  }


  void setUp()
  {

  }

  void tearDown()
  {
  }


  void test_performance_NoBackground()
  {
    for (size_t i=0; i<10; i++)
    {
      MDEWPeakIntegrationTest::doRun(0.02, 0.0);
    }
    // All peaks should be at least 1000 counts (some might be more if they overla)
    for (size_t i=0; i<numPeaks; i += 7)
    {
      double expected=2000.0;
      if ((i % 21) == 0)
          expected += 10000.0;
      TS_ASSERT_LESS_THAN(expected-1, peakWS->getPeak(int(i)).getIntensity());
    }
  }

  void test_performance_WithBackground()
  {
    for (size_t i=0; i<10; i++)
    {
      MDEWPeakIntegrationTest::doRun(0.02, 0.03);
    }
  }

};



#endif /* MANTID_MDEVENTS_MDEWPEAKINTEGRATIONTEST_H_ */

