#ifndef MANTID_MD_CONVERT2_QXYZ_DE_TEST_H_
#define MANTID_MD_CONVERT2_QXYZ_DE_TEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDAlgorithms/ConvertToQ3DdE.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;
using namespace Mantid::MDAlgorithms;
using namespace Mantid::MDEvents;

class ConvertToQ3DdETest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    ConvertToQ3DdE alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  ///** Test various combinations of OutputDimensions parameter */
  //void t__t_OutputDimensions_Parameter()
  //{
  //  EventWorkspace_sptr in_ws = Mantid::MDEvents::MDEventsTestHelper::createDiffractionEventWorkspace(10);
  //  AnalysisDataService::Instance().addOrReplace("testInEW", in_ws);
  //  Algorithm_sptr alg;

  //  alg = AlgorithmHelper::runAlgorithm("MakeDiffractionMDEventWorkspace", 6,
  //      "InputWorkspace", "testInEW",
  //      "OutputWorkspace", "testOutMD",
  //      "OutputDimensions", "Q (lab frame)");
  //  TS_ASSERT( alg->isExecuted() );

  //  MDEventWorkspace3Lean::sptr ws;
  //  TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve("testOutMD")) );
  //  TS_ASSERT(ws);
  //  if (!ws) return;
  //  TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "Q_lab_x");

  //  // But you can't add to an existing one of the wrong dimensions type
  //  alg = AlgorithmHelper::runAlgorithm("MakeDiffractionMDEventWorkspace", 6,
  //      "InputWorkspace", "testInEW",
  //      "OutputWorkspace", "testOutMD",
  //      "OutputDimensions", "HKL");
  //  TS_ASSERT( !alg->isExecuted() );

  //  // Let's remove the old workspace and try again - it will work.
  //  AnalysisDataService::Instance().remove("testOutMD");
  //  alg = AlgorithmHelper::runAlgorithm("MakeDiffractionMDEventWorkspace", 6,
  //      "InputWorkspace", "testInEW",
  //      "OutputWorkspace", "testOutMD",
  //      "OutputDimensions", "HKL");
  //  TS_ASSERT( alg->isExecuted() );

  //  TS_ASSERT_THROWS_NOTHING( ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve("testOutMD")) );
  //  TS_ASSERT(ws);
  //  if (!ws) return;
  //  TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "H");
  //}




  //void do_test_MINITOPAZ(EventType type, size_t numTimesToAdd = 1)
  //{

  //  int numEventsPer = 100;
  //  EventWorkspace_sptr in_ws = Mantid::MDEvents::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer);
  //  if (type == WEIGHTED)
  //    in_ws *= 2.0;
  //  if (type == WEIGHTED_NOTIME)
  //  {
  //    for (size_t i =0; i<in_ws->getNumberHistograms(); i++)
  //    {
  //      EventList & el = in_ws->getEventList(i);
  //      el.compressEvents(0.0, &el);
  //    }
  //  }

  //  ConvertToQ3DdE alg;
  //  TS_ASSERT_THROWS_NOTHING( alg.initialize() )
  //  TS_ASSERT( alg.isInitialized() )
  //  alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(in_ws) );
  //  alg.setPropertyValue("OutputWorkspace", "test_md3");
  //  TS_ASSERT_THROWS_NOTHING( alg.execute(); )
  //  TS_ASSERT( alg.isExecuted() )

  //  MDEventWorkspace3Lean::sptr ws;
  //  TS_ASSERT_THROWS_NOTHING(
  //      ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve("test_md3")) );
  //  TS_ASSERT(ws);
  //  if (!ws) return;
  //  size_t npoints = ws->getNPoints();
  //  TS_ASSERT_LESS_THAN( 100000, npoints); // Some points are left

  //  // Add to an existing MDEW
  //  for (size_t i=1; i < numTimesToAdd; i++)
  //  {
  //    std::cout << "Iteration " << i << std::endl;
  //    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
  //    TS_ASSERT( alg.isInitialized() )
  //    alg.setProperty("InputWorkspace", in_ws);
  //    alg.setPropertyValue("OutputWorkspace", "test_md3");
  //    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
  //    TS_ASSERT( alg.isExecuted() )

  //    TS_ASSERT_THROWS_NOTHING(
  //        ws = boost::dynamic_pointer_cast<MDEventWorkspace3Lean>(AnalysisDataService::Instance().retrieve("test_md3")) );
  //    TS_ASSERT(ws);
  //    if (!ws) return;

  //    TS_ASSERT_EQUALS( npoints*(i+1), ws->getNPoints()); // There are now twice as many points as before
  //  }



  //  AnalysisDataService::Instance().remove("test_md3");
  //}

  //void t__t_MINITOPAZ()
  //{
  //  do_test_MINITOPAZ(TOF);
  //}



};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

