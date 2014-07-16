#ifndef MANTID_CRYSTAL_AnvredCorrectionTEST_H_
#define MANTID_CRYSTAL_AnvredCorrectionTEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidCrystal/AnvredCorrection.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/FacilityHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidDataHandling/RotateInstrumentComponent.h"
#include "MantidDataHandling/MoveInstrumentComponent.h"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <math.h>
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;


class AnvredCorrectionTest : public CxxTest::TestSuite
{
public:

  /** Create an EventWorkspace containing fake data
   * of single-crystal diffraction.
   *
   * @return EventWorkspace_sptr
   */
  EventWorkspace_sptr createDiffractionEventWorkspace()
  {
	// setup the test workspace
	EventWorkspace_sptr retVal = WorkspaceCreationHelper::createEventWorkspaceWithFullInstrument(1,1,false);

    MoveInstrumentComponent mover;
    mover.initialize();
    mover.setProperty("Workspace", retVal);
    mover.setPropertyValue("ComponentName","bank1(x=0)");
    mover.setPropertyValue("X","0.5");
    mover.setPropertyValue("Y","0.");
    mover.setPropertyValue("Z","-5");
    mover.setPropertyValue("RelativePosition","1");
    mover.execute();
    double angle = -90.0;
    Mantid::Kernel::V3D axis(0.,1.,0.);
	RotateInstrumentComponent alg;
	alg.initialize();
	alg.setChild(true);
	TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", retVal));
	TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("ComponentName", "bank1(x=0)") );
	TS_ASSERT_THROWS_NOTHING(alg.setProperty("X", axis.X()) );
	TS_ASSERT_THROWS_NOTHING(alg.setProperty("Y", axis.Y()) );
	TS_ASSERT_THROWS_NOTHING(alg.setProperty("Z", axis.Z()) );
	TS_ASSERT_THROWS_NOTHING(alg.setProperty("Angle", angle) );
	TS_ASSERT_THROWS_NOTHING(alg.setProperty("RelativeRotation", false) );
	TS_ASSERT_THROWS_NOTHING( alg.execute(); );
	TS_ASSERT( alg.isExecuted() );

    return retVal;
  }

  void test_Init()
  {
    AnvredCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  void do_test_events(bool ev)
  {

    MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace();
    inputW->getAxis(0)->setUnit("Wavelength");

    AnvredCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("OutputWorkspace", "TOPAZ");
    alg.setProperty("PreserveEvents", ev);
    alg.setProperty("OnlySphericalAbsorption", false);
    alg.setProperty("LinearScatteringCoef", 0.369);
    alg.setProperty("LinearAbsorptionCoef", 0.011);
    alg.setProperty("Radius", 0.05);
    alg.setProperty("PowerLambda", 3.0);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    MatrixWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("TOPAZ") );
    TS_ASSERT(ws);
    if (!ws) return;
    // do the final comparison
    const MantidVec & y_actual = ws->readY(0);
    TS_ASSERT_DELTA(y_actual[0], 8.2052, 0.0001);
    TS_ASSERT_DELTA(y_actual[1], 0.3040, 0.0001);
    TS_ASSERT_DELTA(y_actual[2], 0.0656, 0.0001);

    AnalysisDataService::Instance().remove("TOPAZ");
  }

  void test_events()
  {
    do_test_events(true);
    do_test_events(false);
  }



};


#endif /* MANTID_CRYSTAL_AnvredCorrectionTEST_H_ */
