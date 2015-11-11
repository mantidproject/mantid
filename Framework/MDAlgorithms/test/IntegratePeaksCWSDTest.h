#ifndef MANTID_MDAGORITHMS_INTEGRATEPEAKSCWSDTEST_H_
#define MANTID_MDAGORITHMS_INTEGRATEPEAKSCWSDTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/HKL.h"

#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidMDAlgorithms/IntegratePeaksCWSD.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/fpclassify.hpp>
#include <boost/math/special_functions/pow.hpp>
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using Mantid::API::AnalysisDataService;
using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;
using Mantid::Kernel::V3D;

class IntegratePeaksCWSDTest : public CxxTest::TestSuite {
public:
  IntegratePeaksCWSDTest() { Mantid::API::FrameworkManager::Instance(); }
  ~IntegratePeaksCWSDTest() {}

  void test_Init() {
    IntegratePeaksCWSD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Run the IntegratePeaksCWSD with the given peak radius integration param */
  static void doRun(double PeakRadius, double BackgroundRadius,
                    std::string OutputWorkspace = "IntegratePeaksCWSDTest_peaks",
                    double BackgroundStartRadius = 0.0, bool edge = true,
                    bool cyl = false, std::string fnct = "NoFit",
                    double adaptive = 0.0) {
    IntegratePeaksCWSD alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspace", "IntegratePeaksCWSDTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PeakRadius", PeakRadius));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BackgroundOuterRadius", BackgroundRadius));
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("BackgroundInnerRadius", BackgroundStartRadius));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrateIfOnEdge", edge));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("PeaksWorkspace", "IntegratePeaksCWSDTest_peaks"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OutputWorkspace));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Cylinder", cyl));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("CylinderLength", 4.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PercentBackground", 20.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("ProfileFunction", fnct));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("IntegrationOption", "Sum"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQMultiplier", adaptive));
    if (adaptive > 0.0)
      TS_ASSERT_THROWS_NOTHING(alg.setProperty("AdaptiveQBackground", true));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Create the (blank) MDEW */
  static void createMDEW() {
    // ---- Start with empty MDEW ----

    CreateMDWorkspace algC;
    TS_ASSERT_THROWS_NOTHING(algC.initialize())
    TS_ASSERT(algC.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Dimensions", "3"));
    TS_ASSERT_THROWS_NOTHING(
        algC.setProperty("Extents", "-10,10,-10,10,-10,10"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Names", "h,k,l"));
    std::string units = Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii();
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", units));
    std::string frames = Mantid::Geometry::HKL::HKLName + "," +
                         Mantid::Geometry::HKL::HKLName + "," +
                         Mantid::Geometry::HKL::HKLName;
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", frames));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "2"));
    TS_ASSERT_THROWS_NOTHING(algC.setPropertyValue(
        "OutputWorkspace", "IntegratePeaksCWSDTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algC.execute());
    TS_ASSERT(algC.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Add a fake peak */
  static void addPeak(size_t num, double x, double y, double z, double radius) {
    std::ostringstream mess;
    mess << num << ", " << x << ", " << y << ", " << z << ", " << radius;
    FakeMDEventData algF;
    TS_ASSERT_THROWS_NOTHING(algF.initialize())
    TS_ASSERT(algF.isInitialized())
    TS_ASSERT_THROWS_NOTHING(
        algF.setPropertyValue("InputWorkspace", "IntegratePeaksCWSDTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("PeakParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

//=========================================================================================
class IntegratePeaksCWSDTestPerformance : public CxxTest::TestSuite {
public:
  size_t numPeaks;
  PeaksWorkspace_sptr peakWS;

  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static IntegratePeaksCWSDTestPerformance *createSuite() {
    return new IntegratePeaksCWSDTestPerformance();
  }
  static void destroySuite(IntegratePeaksCWSDTestPerformance *suite) {
    delete suite;
  }

  IntegratePeaksCWSDTestPerformance() {
    numPeaks = 1000;
    // Original MDEW.
    IntegratePeaksCWSDTest::createMDEW();

    // Add a uniform, random background.

    FakeMDEventData algF2;
    TS_ASSERT_THROWS_NOTHING(algF2.initialize())
    TS_ASSERT(algF2.isInitialized())
    TS_ASSERT_THROWS_NOTHING(algF2.setPropertyValue(
        "InputWorkspace", "IntegratePeaksCWSDTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(algF2.setProperty("UniformParams", "100000"));
    TS_ASSERT_THROWS_NOTHING(algF2.execute());
    TS_ASSERT(algF2.isExecuted());

    MDEventWorkspace3Lean::sptr mdews =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "IntegratePeaksCWSDTest_MDEWS");
    mdews->setCoordinateSystem(Mantid::Kernel::HKL);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);

    boost::mt19937 rng;
    boost::uniform_real<double> u(-9.0, 9.0); // Random from -9 to 9.0
    boost::variate_generator<boost::mt19937 &, boost::uniform_real<double>> gen(
        rng, u);

    peakWS = PeaksWorkspace_sptr(new PeaksWorkspace());
    for (size_t i = 0; i < numPeaks; ++i) {
      // Random peak center
      double x = gen();
      double y = gen();
      double z = gen();

      // Make the peak
      IntegratePeaksCWSDTest::addPeak(1000, x, y, z, 0.02);
      // With a center with higher density. 2000 events total.
      IntegratePeaksCWSDTest::addPeak(1000, x, y, z, 0.005);

      // Make a few very strong peaks
      if (i % 21 == 0)
        IntegratePeaksCWSDTest::addPeak(10000, x, y, z, 0.015);

      // Add to peaks workspace
      peakWS->addPeak(Peak(inst, 1, 1.0, V3D(x, y, z)));

      if (i % 100 == 0)
        std::cout << "Peak " << i << " added\n";
    }
    AnalysisDataService::Instance().add("IntegratePeaksCWSDTest_peaks", peakWS);
  }

  ~IntegratePeaksCWSDTestPerformance() {
    AnalysisDataService::Instance().remove("IntegratePeaksCWSDTest_MDEWS");
    AnalysisDataService::Instance().remove("IntegratePeaksCWSDTest_peaks");
  }

  void setUp() {}

  void tearDown() {}

  void test_performance_NoBackground() {
    for (size_t i = 0; i < 10; i++) {
      IntegratePeaksCWSDTest::doRun(0.02, 0.0);
    }
    // All peaks should be at least 1000 counts (some might be more if they
    // overla)
    for (size_t i = 0; i < numPeaks; i += 7) {
      double expected = 2000.0;
      if ((i % 21) == 0)
        expected += 10000.0;
      TS_ASSERT_LESS_THAN(expected - 1, peakWS->getPeak(int(i)).getIntensity());
    }
  }

  void test_performance_WithBackground() {
    for (size_t i = 0; i < 10; i++) {
      IntegratePeaksCWSDTest::doRun(0.02, 0.03);
    }
  }
};

#endif /* MANTID_MDEVENTS_INTEGRATEPEAKSCWSDTEST_H_ */
