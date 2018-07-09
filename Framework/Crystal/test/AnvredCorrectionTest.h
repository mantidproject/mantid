#ifndef MANTID_CRYSTAL_AnvredCorrectionTEST_H_
#define MANTID_CRYSTAL_AnvredCorrectionTEST_H_

#include "MantidAPI/Axis.h"
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <math.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

// Anonymous namespace to share methods between Unit and performance test
namespace {
/** Create an EventWorkspace containing fake data
 * of single-crystal diffraction.
 *
 * @return EventWorkspace_sptr
 */
EventWorkspace_sptr createDiffractionEventWorkspace(int numBanks = 1,
                                                    int numPixels = 1) {
  // setup the test workspace
  EventWorkspace_sptr retVal =
      WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(
          numBanks, numPixels, false);

  MoveInstrumentComponent mover;
  mover.initialize();
  mover.setProperty("Workspace", retVal);
  mover.setPropertyValue("ComponentName", "bank1(x=0)");
  mover.setPropertyValue("X", "0.5");
  mover.setPropertyValue("Y", "0.");
  mover.setPropertyValue("Z", "-5");
  mover.setPropertyValue("RelativePosition", "1");
  mover.execute();
  double angle = -90.0;
  Mantid::Kernel::V3D axis(0., 1., 0.);
  RotateInstrumentComponent alg;
  alg.initialize();
  alg.setChild(true);
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", retVal));
  TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ComponentName", "bank1(x=0)"));
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("X", axis.X()));
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("Y", axis.Y()));
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("Z", axis.Z()));
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("Angle", angle));
  TS_ASSERT_THROWS_NOTHING(alg.setProperty("RelativeRotation", false));
  TS_ASSERT_THROWS_NOTHING(alg.execute(););
  TS_ASSERT(alg.isExecuted());

  return retVal;
}

void do_test_events(MatrixWorkspace_sptr workspace, bool ev,
                    bool performance = false) {

  workspace->getAxis(0)->setUnit("Wavelength");

  AnvredCorrection alg;
  TS_ASSERT_THROWS_NOTHING(alg.initialize())
  TS_ASSERT(alg.isInitialized())
  alg.setProperty("InputWorkspace", workspace);
  alg.setProperty("OutputWorkspace", "TOPAZ");
  alg.setProperty("PreserveEvents", ev);
  alg.setProperty("OnlySphericalAbsorption", false);
  alg.setProperty("LinearScatteringCoef", 0.369);
  alg.setProperty("LinearAbsorptionCoef", 0.011);
  alg.setProperty("Radius", 0.05);
  alg.setProperty("PowerLambda", 3.0);
  TS_ASSERT_THROWS_NOTHING(alg.execute();)
  TS_ASSERT(alg.isExecuted())

  if (!performance) {
    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "TOPAZ"));
    TS_ASSERT(ws);
    if (!ws)
      return;
    // do the final comparison
    const auto &y_actual = ws->y(0);
    TS_ASSERT_DELTA(y_actual[0], 8.2052, 0.0001);
    TS_ASSERT_DELTA(y_actual[1], 0.3040, 0.0001);
    TS_ASSERT_DELTA(y_actual[2], 0.0656, 0.0001);
  }
}
} // namespace

class AnvredCorrectionTest : public CxxTest::TestSuite {
public:
  static AnvredCorrectionTest *createSuite() {
    return new AnvredCorrectionTest();
  }
  static void destroySuite(AnvredCorrectionTest *suite) { delete suite; }

  void test_Init() {
    AnvredCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void setUp() override { workspace = createDiffractionEventWorkspace(); }

  void tearDown() override { AnalysisDataService::Instance().remove("TOPAZ"); }

  void test_Events() { do_test_events(workspace, true); }

  void test_NoEvents() { do_test_events(workspace, false); }

private:
  EventWorkspace_sptr workspace;
};

class AnvredCorrectionTestPerformance : public CxxTest::TestSuite {
public:
  static AnvredCorrectionTestPerformance *createSuite() {
    return new AnvredCorrectionTestPerformance();
  }
  static void destroySuite(AnvredCorrectionTestPerformance *suite) {
    delete suite;
  }

  // executed before each test
  void setUp() override { workspace = createDiffractionEventWorkspace(100, 5); }

  // executed after each test
  void tearDown() override { AnalysisDataService::Instance().remove("TOPAZ"); }

  void testEventsPerformance() { do_test_events(workspace, true, performance); }

  void testNoEventsPerformace() {
    do_test_events(workspace, false, performance);
  }

private:
  const bool performance = true;
  EventWorkspace_sptr workspace;
};

#endif /* MANTID_CRYSTAL_AnvredCorrectionTEST_H_ */
