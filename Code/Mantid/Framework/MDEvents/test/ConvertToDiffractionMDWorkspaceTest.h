#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_

#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidMDEvents/ConvertToDiffractionMDWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>
#include "MantidAPI/IAlgorithm.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::MDEvents;

class ConvertToDiffractionMDWorkspaceTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    ConvertToDiffractionMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  /** Test various combinations of OutputDimensions parameter */
  void test_OutputDimensions_Parameter()
  {
    EventWorkspace_sptr in_ws = Mantid::MDEvents::MDEventsTestHelper::createDiffractionEventWorkspace(10);
    AnalysisDataService::Instance().addOrReplace("testInEW", in_ws);
    IAlgorithm_sptr alg;

    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace", 6,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "OutputDimensions", "Q (lab frame)");
    TS_ASSERT( alg->isExecuted() );

    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("testOutMD") );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "Q_lab_x");

    // But you can't add to an existing one of the wrong dimensions type, if you choose Append
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace", 8,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "Append", "1",
        "OutputDimensions", "HKL");
    TS_ASSERT( !alg->isExecuted() );

    // If Append is False, then it does work. The workspace gets replaced
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace", 8,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "Append", "0",
        "OutputDimensions", "HKL");
    TS_ASSERT( alg->isExecuted() );

    // Let's remove the old workspace and try again - it will work.
    AnalysisDataService::Instance().remove("testOutMD");
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace", 8,
        "InputWorkspace", "testInEW",
        "OutputWorkspace", "testOutMD",
        "Append", "1",
        "OutputDimensions", "HKL");
    TS_ASSERT( alg->isExecuted() );

    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("testOutMD") );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "H");
  }




  void do_test_MINITOPAZ(EventType type, size_t numTimesToAdd = 1)
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

    ConvertToDiffractionMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", boost::dynamic_pointer_cast<MatrixWorkspace>(in_ws) );
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
    TS_ASSERT_EQUALS( ws->getNumExperimentInfo(), 1);
    TSM_ASSERT("ExperimentInfo object is valid", ws->getExperimentInfo(0) );

    // Add to an existing MDEW
    for (size_t i=1; i < numTimesToAdd; i++)
    {
      std::cout << "Iteration " << i << std::endl;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
      alg.setProperty("InputWorkspace", in_ws);
      alg.setProperty("Append", true);
      alg.setPropertyValue("OutputWorkspace", "test_md3");
      TS_ASSERT_THROWS_NOTHING( alg.execute(); )
      TS_ASSERT( alg.isExecuted() )

      TS_ASSERT_THROWS_NOTHING(
          ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("test_md3") );
      TS_ASSERT(ws);
      if (!ws) return;

      TS_ASSERT_EQUALS( npoints*(i+1), ws->getNPoints()); // There are now twice as many points as before
      TS_ASSERT_EQUALS( ws->getNumExperimentInfo(), (i+1));
      TSM_ASSERT("ExperimentInfo object is valid", ws->getExperimentInfo(static_cast<uint16_t>(i)) );
    }



    AnalysisDataService::Instance().remove("test_md3");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }
//
//  void test_MINITOPAZ_weightedEvents()
//  {
//    do_test_MINITOPAZ(WEIGHTED);
//  }
//
//  void test_MINITOPAZ_weightedEvents_noTime()
//  {
//    do_test_MINITOPAZ(WEIGHTED);
//  }
//
  void test_MINITOPAZ_addToExistingWorkspace()
  {
    do_test_MINITOPAZ(TOF, 2);
  }
//
//  void test_MINITOPAZ_forProfiling()
//  {
//    do_test_MINITOPAZ(TOF, 100);
//  }



};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

