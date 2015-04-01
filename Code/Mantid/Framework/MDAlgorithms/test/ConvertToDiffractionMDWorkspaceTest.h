#ifndef MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_
#define MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidMDAlgorithms/ConvertToDiffractionMDWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataObjects;
using namespace Mantid::MDAlgorithms;

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
    EventWorkspace_sptr in_ws = Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(10);
    AnalysisDataService::Instance().addOrReplace("testInEW", in_ws);
    IAlgorithm* alg;

    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
        "InputWorkspace=testInEW;OutputWorkspace=testOutMD;OutputDimensions=Q (lab frame)",1);
    TS_ASSERT( alg->isExecuted() );

    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("testOutMD") );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "Q_lab_x");
    TS_ASSERT_EQUALS( ws->getSpecialCoordinateSystem(), Mantid::Kernel::QLab);

    // But you can't add to an existing one of the wrong dimensions type, if you choose Append
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
        "InputWorkspace=testInEW;"
        "OutputWorkspace=testOutMD;"
        "Append=1;"
        "OutputDimensions=HKL",1);
    TS_ASSERT( !alg->isExecuted() );

    // If Append is False, then it does work. The workspace gets replaced
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace", 
        "InputWorkspace=testInEW;"
        "OutputWorkspace=testOutMD;"
        "Append=0;"
        "OutputDimensions=HKL",1);
    TS_ASSERT( alg->isExecuted() );

    // Let's remove the old workspace and try again - it will work.
    AnalysisDataService::Instance().remove("testOutMD");
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
        "InputWorkspace=testInEW;"
        "OutputWorkspace=testOutMD;"
        "Append=1;"
        "OutputDimensions=HKL",1);
    TS_ASSERT( alg->isExecuted() );

    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("testOutMD") );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "H");
    TS_ASSERT_EQUALS( ws->getSpecialCoordinateSystem(), Mantid::Kernel::HKL);

    AnalysisDataService::Instance().remove("testOutMD");
    alg = FrameworkManager::Instance().exec("ConvertToDiffractionMDWorkspace",
        "InputWorkspace=testInEW;"
        "OutputWorkspace=testOutMD;"
        "OutputDimensions=Q (sample frame)",1
        );
    TS_ASSERT( alg->isExecuted() );

    TS_ASSERT_THROWS_NOTHING( ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("testOutMD") );
    TS_ASSERT(ws);
    if (!ws) return;
    TS_ASSERT_EQUALS( ws->getDimension(0)->getName(), "Q_sample_x");
    TS_ASSERT_EQUALS( ws->getSpecialCoordinateSystem(), Mantid::Kernel::QSample);
  }

  void do_test_MINITOPAZ(EventType type, size_t numTimesToAdd = 1,
      bool OneEventPerBin=false, bool MakeWorkspace2D = false)
  {

    int numEventsPer = 100;
    EventWorkspace_sptr in_ws = Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer);
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

    // Rebin the workspace to have a manageable number bins
    AnalysisDataService::Instance().addOrReplace("inputWS", in_ws);
    FrameworkManager::Instance().exec("Rebin", 8,
        "InputWorkspace", "inputWS",
        "OutputWorkspace", "inputWS",
        "Params", "0, 500, 16e3",
        "PreserveEvents", MakeWorkspace2D ? "0" : "1" );

    ConvertToDiffractionMDWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setPropertyValue("InputWorkspace", "inputWS");
    alg.setProperty("OneEventPerBin", OneEventPerBin);
    alg.setPropertyValue("OutputWorkspace", "test_md3");
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    MDEventWorkspace3Lean::sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<MDEventWorkspace3Lean>("test_md3") );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t npoints = ws->getNPoints();
    // # of points != # of bins exactly because some are off the extents
    TS_ASSERT_LESS_THAN( 399, npoints);

    TS_ASSERT_EQUALS( ws->getNumExperimentInfo(), 1);
    TSM_ASSERT("ExperimentInfo object is valid", ws->getExperimentInfo(0) );

    // Add to an existing MDEW
    for (size_t i=1; i < numTimesToAdd; i++)
    {
      std::cout << "Iteration " << i << std::endl;
      TS_ASSERT_THROWS_NOTHING( alg.initialize() )
      TS_ASSERT( alg.isInitialized() )
      alg.setPropertyValue("InputWorkspace", "inputWS");
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

  void test_MINITOPAZ_Weighted()
  {
    do_test_MINITOPAZ(WEIGHTED);
  }

  void test_MINITOPAZ_addToExistingWorkspace()
  {
    do_test_MINITOPAZ(TOF, 2);
  }

  void test_MINITOPAZ_OneEventPerBin_fromEventWorkspace()
  {
    do_test_MINITOPAZ(TOF, 1, true, false);
  }

  void test_MINITOPAZ_OneEventPerBin_fromWorkspace2D()
  {
    do_test_MINITOPAZ(TOF, 1, true, true);
  }

  void test_MINITOPAZ_fromWorkspace2D()
  {
    do_test_MINITOPAZ(TOF, 1, false, true);
  }




};


#endif /* MANTID_MDEVENTS_MAKEDIFFRACTIONMDEVENTWORKSPACETEST_H_ */

