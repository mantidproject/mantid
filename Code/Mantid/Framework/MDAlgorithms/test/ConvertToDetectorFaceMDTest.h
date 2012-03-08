#ifndef MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMDTEST_H_
#define MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMDTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include <iostream>
#include <iomanip>

#include "MantidMDAlgorithms/ConvertToDetectorFaceMD.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidMDEvents/MDEventFactory.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"

using namespace Mantid;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;

class ConvertToDetectorFaceMDTest : public CxxTest::TestSuite
{
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ConvertToDetectorFaceMDTest *createSuite() { return new ConvertToDetectorFaceMDTest(); }
  static void destroySuite( ConvertToDetectorFaceMDTest *suite ) { delete suite; }


  void test_Init()
  {
    ConvertToDetectorFaceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }


  void do_test_MINITOPAZ(EventType type)
  {

    int numEventsPer = 100;
    EventWorkspace_sptr in_ws = Mantid::MDEvents::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer);
    if (type == WEIGHTED)
      in_ws *= 2.0;
    if (type == WEIGHTED_NOTIME)
    {
      for (size_t i =0; i<in_ws->getNumberHistograms(); i++)
      {
        EventList & el = in_ws->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }

    ConvertToDetectorFaceMD alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(in_ws) );
    alg.setPropertyValue("BankNumbers", "1");
    alg.setPropertyValue("OutputWorkspace", "test_md3");
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("test_md3") );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t npoints = ws->getNPoints();
    TS_ASSERT_LESS_THAN( 100000, npoints); // Some points are left

    AnalysisDataService::Instance().remove("test_md3");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }
  

};


#endif /* MANTID_MDALGORITHMS_CONVERTTODETECTORFACEMDTEST_H_ */
