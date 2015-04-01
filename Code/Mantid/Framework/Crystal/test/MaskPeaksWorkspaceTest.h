#ifndef MANTID_DATAHANDLING_MaskPeaksWorkspaceTEST_H_
#define MANTID_DATAHANDLING_MaskPeaksWorkspaceTEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidCrystal/MaskPeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include "MantidDataObjects/EventList.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Crystal;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::DataHandling;

class MaskPeaksWorkspaceTest : public CxxTest::TestSuite
{
public:
    
  void test_Init()
  {
    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
  }
  


  void do_test_MINITOPAZ(EventType type)
  {

    int numEventsPer = 100;
    EventWorkspace_sptr inputW = Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer,10000,1600);
    AnalysisDataService::Instance().addOrReplace("testInEW", inputW);
    if (type == WEIGHTED)
      inputW *= 2.0;
    if (type == WEIGHTED_NOTIME)
    {
      for (size_t i =0; i<inputW->getNumberHistograms(); i++)
      {
        EventList & el = inputW->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }
    size_t nevents0 = inputW->getNumberEvents();
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    //pkws->setName("TOPAZ");

    // This loads (appends) the peaks
    Mantid::DataObjects::Peak PeakObj(inputW->getInstrument(),1000,100.);
    pkws->addPeak( PeakObj);
    AnalysisDataService::Instance().add("TOPAZ", pkws);

    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("InPeaksWorkspace", "TOPAZ");
    alg.setProperty("XMin", -2);
    alg.setProperty("XMax", 2);
    alg.setProperty("YMin", -2);
    alg.setProperty("YMax", 2);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("testInEW") );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t nevents = ws->getNumberEvents();
    TS_ASSERT_LESS_THAN( nevents, nevents0); 

    AnalysisDataService::Instance().remove("testInEW");
    AnalysisDataService::Instance().remove("TOPAZ");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }

  /*
   * Test make peaks with ralative TOF range
   */
  void do_test_TOFRange(EventType type)
  {

    int numEventsPer = 100;
    EventWorkspace_sptr inputW = Mantid::DataObjects::MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer,10000,1600);
    AnalysisDataService::Instance().addOrReplace("testInEW", inputW);
    if (type == WEIGHTED)
    {
      inputW *= 2.0;
    }
    if (type == WEIGHTED_NOTIME)
    {
      for (size_t i =0; i<inputW->getNumberHistograms(); i++)
      {
        EventList & el = inputW->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }
    size_t nevents0 = inputW->getNumberEvents();
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    //pkws->setName("TOPAZ");

    // This loads (appends) the peaks
    Mantid::DataObjects::Peak PeakObj(inputW->getInstrument(),1000, 1.);
    pkws->addPeak( PeakObj);
    AnalysisDataService::Instance().add("TOPAZ2", pkws);

    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    alg.setProperty("InputWorkspace", inputW);
    alg.setProperty("InPeaksWorkspace", "TOPAZ2");
    alg.setProperty("XMin", -2);
    alg.setProperty("XMax", 2);
    alg.setProperty("YMin", -2);
    alg.setProperty("YMax", 2);
    alg.setProperty("TOFMin", -2500.0);
    alg.setProperty("TOFMax", 5000.0);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("testInEW") );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t nevents = ws->getNumberEvents();
    TS_ASSERT_LESS_THAN( nevents, nevents0);
    TS_ASSERT_LESS_THAN( 999400, nevents);

    AnalysisDataService::Instance().remove("testInEW");
    AnalysisDataService::Instance().remove("TOPAZ2");

    return;
  }

  void test_TOFRange()
  {
    do_test_TOFRange(TOF);
  }

};


#endif /* MANTID_DATAHANDLING_MaskPeaksWorkspaceTEST_H_ */

