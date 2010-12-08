#ifndef MEMORYLEAKTEST_H
#define MEMORYLEAKTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>
#include "../../Algorithms/test/WorkspaceCreationHelper.hh"
#include "MantidNexus/LoadSNSEventNexus.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Workspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidNexus/LoadLogsFromSNSNexus.h"

using namespace Mantid;
using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::NeXus;
//using namespace Mantid::DataObjects;


class MemoryLeakTest : public CxxTest::TestSuite
{
public:

//  void testLeak1()
//  {
//    int numPixels = 10000;
//    int numEvents = 1000;
//    EventWorkspace_sptr ew = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numEvents, numEvents);
//    AnalysisDataService::Instance().addOrReplace("ew1", ew);
//  }
//
//  void testLeak2()
//  {
//    int numPixels = 10000;
//    int numEvents = 1000;
//    EventWorkspace_sptr ew2 = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numEvents, numEvents);
//    //Overwrite
//    AnalysisDataService::Instance().addOrReplace("ew1", ew2);
//  }

  // THIS DOES NOT LEAK
  void xtestLeak3()
  {
    for (int i=0; i<50; i++)
    {
      std::cout << "Creating event workspace " << i << "\n";
      int numPixels = 10000;
      int numEvents = 1000;
      EventWorkspace_sptr ew2 = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numEvents, numEvents);
      //Overwrite
      AnalysisDataService::Instance().addOrReplace("ew1", ew2);
    }
  }


  // THIS DOES NOT LEAK
  void xtest_MemoryLeak_inLoadSNSEventNexus()
  {

    for (int i=0; i<50; i++)
    {
      std::cout << "LoadSNSEventNexus call # " << i << "\n";
      LoadSNSEventNexus ld;
      std::string outws_name = "cncs";
      ld.initialize();
      ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");
      ld.setPropertyValue("OutputWorkspace",outws_name);
      ld.setPropertyValue("FilterByTof_Min", "-1e6");
      ld.setPropertyValue("FilterByTof_Max", "1e6");
      ld.setPropertyValue("FilterByTime_Start", "-1e6");
      ld.setPropertyValue("FilterByTime_Stop", "1e6");

      ld.execute();
      TS_ASSERT( ld.isExecuted() );

      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
      //Valid WS and it is an EventWorkspace
      TS_ASSERT( WS );
      //Events
      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
    }
  }

  // ! THIS LEAKS !
  void xtest_MemoryLeak_inLoadSNSEventNexus2()
  {
    for (int i=0; i<50; i++)
    {
      std::cout << "LoadSNSEventNexus call # " << i << "\n";
      IAlgorithm_sptr ld = AlgorithmFactory::Instance().create("LoadSNSEventNexus",1);
      std::string outws_name = "cncs";
      ld->initialize();
      ld->setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");
      ld->setPropertyValue("OutputWorkspace",outws_name);
      ld->setPropertyValue("FilterByTof_Min", "-1e6");
      ld->setPropertyValue("FilterByTof_Max", "1e6");
      ld->setPropertyValue("FilterByTime_Start", "-1e6");
      ld->setPropertyValue("FilterByTime_Stop", "1e6");

      ld->execute();
      TS_ASSERT( ld->isExecuted() );

      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
      //Valid WS and it is an EventWorkspace
      TS_ASSERT( WS );
      //Events
      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
    }
  }

  // ! THIS LEAKS TOO !
  void xtest_MemoryLeak_inLoadSNSEventNexus3()
  {
    for (int i=0; i<50; i++)
    {
      std::cout << "LoadSNSEventNexus call # " << i << "\n";

      //IAlgorithm * ld = AlgorithmFactory::Instance().create("LoadSNSEventNexus");
      IAlgorithm * ld = Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadSNSEventNexus");

      std::string outws_name = "cncs";
      ld->initialize();
      ld->setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");
      ld->setPropertyValue("OutputWorkspace",outws_name);
      ld->setPropertyValue("FilterByTof_Min", "-1e6");
      ld->setPropertyValue("FilterByTof_Max", "1e6");
      ld->setPropertyValue("FilterByTime_Start", "-1e6");
      ld->setPropertyValue("FilterByTime_Stop", "1e6");

      ld->execute();
      TS_ASSERT( ld->isExecuted() );

      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
      //Valid WS and it is an EventWorkspace
      TS_ASSERT( WS );
      //Events
      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
    }
  }

};


#endif /*LOADSNSEVENTNEXUSTEST_H_*/



