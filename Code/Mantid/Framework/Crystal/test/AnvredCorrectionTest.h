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
  EventWorkspace_sptr createDiffractionEventWorkspace(int numEvents)
  {
	// setup the test workspace
	EventWorkspace_sptr retVal = WorkspaceCreationHelper::CreateEventWorkspace2(1, 100);
    // --------- Load the instrument -----------
    LoadInstrument * loadInst = new LoadInstrument();
    loadInst->initialize();
    loadInst->setPropertyValue("Filename", "IDFs_for_UNIT_TESTING/MINITOPAZ_Definition.xml");
    loadInst->setProperty<MatrixWorkspace_sptr> ("Workspace", retVal);
    loadInst->execute();
    delete loadInst;
    // Populate the instrument parameters in this workspace - this works around a bug
    retVal->populateInstrumentParameters();

    return retVal;
  }

  void test_Init()
  {
    AnvredCorrection alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  void do_test_MINITOPAZ(bool ev)
  {

    int numEventsPer = 10;
    MatrixWorkspace_sptr inputW = createDiffractionEventWorkspace(numEventsPer);
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
    // do the final comparison - this is done by bounding
    const MantidVec & y_actual = ws->readY(0);
    TS_ASSERT_DELTA(y_actual[0], 4.8477, 0.4);
    TS_ASSERT_DELTA(y_actual[1], 0.1796, 0.02);
    TS_ASSERT_DELTA(y_actual[2], 0.0388, 0.004);

    AnalysisDataService::Instance().remove("TOPAZ");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(true);
    do_test_MINITOPAZ(false);
  }



};


#endif /* MANTID_CRYSTAL_AnvredCorrectionTEST_H_ */
