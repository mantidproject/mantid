#ifndef MANTID_DATAHANDLING_MaskPeaksWorkspaceTEST_H_
#define MANTID_DATAHANDLING_MaskPeaksWorkspaceTEST_H_

#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/MaskPeaksWorkspace.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidKernel/System.h"
#include "MantidKernel/Timer.h"
#include "MantidTestHelpers/AlgorithmHelper.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidTestHelpers/MDEventsTestHelper.h"
#include <cxxtest/TestSuite.h>
#include <iomanip>
#include <iostream>

using namespace Mantid;
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
    MatrixWorkspace_sptr inputW = MDEventsTestHelper::createDiffractionEventWorkspace(numEventsPer);
    EventWorkspace_sptr in_ws = boost::dynamic_pointer_cast<EventWorkspace>( inputW );
    AnalysisDataService::Instance().addOrReplace("testInEW", in_ws);
    if (type == WEIGHTED)
      in_ws *= 2.0;
    if (type == WEIGHTED_NOTIME)
    {
      for (int i =0; i<in_ws->getNumberHistograms(); i++)
      {
        EventList & el = in_ws->getEventList(i);
        el.compressEvents(0.0, &el);
      }
    }
    size_t nevents0 = in_ws->getNumberEvents();
    // Register the workspace in the data service

    // Create the peaks workspace
    PeaksWorkspace_sptr pkws(new PeaksWorkspace());
    pkws->setName("TOPAZ");

    // This loads (appends) the peaks
    Mantid::DataObjects::Peak PeakObj(in_ws->getInstrument(),1000,100.);
    pkws->addPeak( PeakObj);
    AnalysisDataService::Instance().add("TOPAZ", pkws);

    MaskPeaksWorkspace alg;
    TS_ASSERT_THROWS_NOTHING( alg.initialize() )
    TS_ASSERT( alg.isInitialized() )
    std::cout << "Before InputWorkspace\n";
    alg.setProperty("InputWorkspace", inputW);
    std::cout << "After  InputWorkspace\n";
    alg.setProperty("InPeaksWorkspace", "TOPAZ");
    alg.setProperty("XMin", -2);
    alg.setProperty("XMax", 2);
    alg.setProperty("YMin", -2);
    alg.setProperty("YMax", 2);
    TS_ASSERT_THROWS_NOTHING( alg.execute(); )
    TS_ASSERT( alg.isExecuted() )

    EventWorkspace_sptr ws;
    TS_ASSERT_THROWS_NOTHING(
        ws = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve("testInEW")) );
    TS_ASSERT(ws);
    if (!ws) return;
    size_t nevents = ws->getNumberEvents();
    TS_ASSERT_LESS_THAN( nevents, nevents0); 

    AnalysisDataService::Instance().remove("testInEW");
  }

  void test_MINITOPAZ()
  {
    do_test_MINITOPAZ(TOF);
  }



};


#endif /* MANTID_DATAHANDLING_MaskPeaksWorkspaceTEST_H_ */

