#ifndef MANTID_MDAGORITHMS_INTEGRATEPEAKSMDHKLTEST_H_
#define MANTID_MDAGORITHMS_INTEGRATEPEAKSMDHKLTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/IMDEventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidDataObjects/MDEventFactory.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/PeakShapeSpherical.h"
#include "MantidGeometry/MDGeometry/MDHistoDimension.h"
#include "MantidGeometry/MDGeometry/HKL.h"
#include "MantidMDAlgorithms/IntegratePeaksMDHKL.h"
#include "MantidMDAlgorithms/CreateMDWorkspace.h"
#include "MantidMDAlgorithms/FakeMDEventData.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidKernel/UnitLabelTypes.h"

#include <boost/math/distributions/normal.hpp>
#include <boost/math/special_functions/pow.hpp>

#include <cxxtest/TestSuite.h>

#include <Poco/File.h>

using Mantid::Geometry::MDHistoDimension;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::MDAlgorithms;
using Mantid::Kernel::V3D;

class IntegratePeaksMDHKLTest : public CxxTest::TestSuite {
public:
  IntegratePeaksMDHKLTest() { Mantid::API::FrameworkManager::Instance(); }
  ~IntegratePeaksMDHKLTest() override {}

  void test_Init() {
    IntegratePeaksMDHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  //-------------------------------------------------------------------------------
  /** Run the IntegratePeaksMDHKL with the given peak radius integration param
   */
  static void
  doRun(std::string OutputWorkspace = "IntegratePeaksMDHKLTest_peaks") {
    IntegratePeaksMDHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspace", "IntegratePeaksMDHKLTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "PeaksWorkspace", "IntegratePeaksMDHKLTest_peaks"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", OutputWorkspace));
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

    TS_ASSERT_THROWS_NOTHING(
        algC.setProperty("Names", "[H,0,0],[0,K,0],[0,0,L]"));
    std::string units = Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii() + "," +
                        Mantid::Kernel::Units::Symbol::RLU.ascii();
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Units", units));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("SplitInto", "5"));
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("MaxRecursionDepth", "2"));
    std::string frames = Mantid::Geometry::HKL::HKLName + "," +
                         Mantid::Geometry::HKL::HKLName + "," +
                         Mantid::Geometry::HKL::HKLName;
    TS_ASSERT_THROWS_NOTHING(algC.setProperty("Frames", frames));
    TS_ASSERT_THROWS_NOTHING(algC.setPropertyValue(
        "OutputWorkspace", "IntegratePeaksMDHKLTest_MDEWS"));
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
    TS_ASSERT_THROWS_NOTHING(algF.setPropertyValue(
        "InputWorkspace", "IntegratePeaksMDHKLTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(
        algF.setProperty("PeakParams", mess.str().c_str()));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("RandomSeed", "63759"));
    TS_ASSERT_THROWS_NOTHING(algF.setProperty("RandomizeSignal", "1"));
    TS_ASSERT_THROWS_NOTHING(algF.execute());
    TS_ASSERT(algF.isExecuted());
  }

  //-------------------------------------------------------------------------------
  /** Full test using faked-out peak data */
  void test_exec() {
    // --- Fake workspace with 3 peaks ------
    createMDEW();
    addPeak(1000, 1., 1., 1., 0.1);
    addPeak(1000, 2., 3., 4., 0.15);
    addPeak(1000, 6., 6., 6., 0.2);

    MDEventWorkspace3Lean::sptr mdews =
        AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>(
            "IntegratePeaksMDHKLTest_MDEWS");
    auto &frame = mdews->getDimension(0)->getMDFrame();
    TSM_ASSERT_EQUALS("Should be HKL", Mantid::Geometry::HKL::HKLName,
                      frame.name());
    TS_ASSERT_EQUALS(mdews->getNPoints(), 3000);
    TS_ASSERT_DELTA(mdews->getBox()->getSignal(), 3021.7071, 1e-2);

    // Make a fake instrument - doesn't matter, we won't use it really
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentRectangular(1, 100, 0.05);

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS0(new PeaksWorkspace());
    peakWS0->setInstrument(inst);
    Peak Pin(inst, 15050, 1.0);
    Pin.setHKL(V3D(1, 1, 1));
    peakWS0->addPeak(Pin);

    TS_ASSERT_EQUALS(peakWS0->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().add("IntegratePeaksMDHKLTest_peaks",
                                        peakWS0);

    // ------------- Integrating with cylinder ------------------------
    doRun("IntegratePeaksMDHKLTest_peaks");

    TS_ASSERT_DELTA(peakWS0->getPeak(0).getIntensity(), 29.4284, 1e-2);

    // Error is also calculated
    TS_ASSERT_DELTA(peakWS0->getPeak(0).getSigmaIntensity(), 5.2813, 1e-2);
  }

  //-------------------------------------------------------------------------------
  /// Integrate background between start/end background radius
  void test_exec_shellBackground() {
    createMDEW();
    /* Create 3 overlapping shells so that density goes like this:
     * r < 1 : density 1.0
     * 1 < r < 2 : density 1/2
     * 2 < r < 3 : density 1/3
     */
    addPeak(1000, 1., 1., 1., 0.1);
    addPeak(1000 * 4, 0., 0., 0.,
            2.0); // 8 times the volume / 4 times the counts = 1/2 density
    addPeak(1000 * 9, 0., 0., 0.,
            3.0); // 27 times the volume / 9 times the counts = 1/3 density

    // --- Make a fake PeaksWorkspace ---
    PeaksWorkspace_sptr peakWS(new PeaksWorkspace());
    Instrument_sptr inst =
        ComponentCreationHelper::createTestInstrumentCylindrical(5);
    Peak Pin(inst, 1, 1.0);
    Pin.setHKL(V3D(1, 1, 1));
    peakWS->addPeak(Pin);
    TS_ASSERT_EQUALS(peakWS->getPeak(0).getIntensity(), 0.0);
    AnalysisDataService::Instance().addOrReplace(
        "IntegratePeaksMDHKLTest_peaks", peakWS);

    // Set background from 2.0 to 3.0.
    IntegratePeaksMDHKL alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspace", "IntegratePeaksMDHKLTest_MDEWS"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "PeaksWorkspace", "IntegratePeaksMDHKLTest_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "OutputWorkspace", "IntegratePeaksMDHKLTest_peaks"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundOuterRadius", 0.2));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("BackgroundInnerRadius", 0.16));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_DELTA(peakWS->getPeak(0).getIntensity(), 29.4275, 0.1);
    // Error is larger, since it is error of peak + error of background
    TSM_ASSERT_DELTA("Error has increased",
                     peakWS->getPeak(0).getSigmaIntensity(), 5.2814, 0.1);
  }
};

#endif /* MANTID_MDEVENTS_INTEGRATEPEAKSMDHKLTEST_H_ */
