#ifndef MEMORYLEAKTEST_H
#define MEMORYLEAKTEST_H

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
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

//#ifndef _WIN32
//  #include <sys/resource.h>
//#endif
#include "boost/date_time/gregorian/gregorian.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

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

  void test_nothing()
  {
   TS_ASSERT( 1);
  }


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
//
//
//
//  //------------------------------------------------------------------------------
//  /// Linux-only method for getting memory usage
//  int memory_usage()
//  {
//    // Linux only memory test
//#ifdef _WIN32
//    //Temporarily disabled for non-linux OSs
//#else
//    char buf[30];
//    snprintf(buf, 30, "/proc/%u/statm", (unsigned)getpid());
//    FILE* pf = fopen(buf, "r");
//    if (pf) {
//        int size; //       total program size
//        fscanf(pf, "%u" /* %u %u %u %u %u"*/, &size/*, &resident, &share, &text, &lib, &data*/);
//        fclose(pf);
//        return size*4; //On my system each number here = 4 kb
//    }
//    fclose(pf);
//#endif
//    return 0;
//  }
//
//  // THIS DOES NOT LEAK after i=2
//  void xtestLeakTSP()
//  {
//    // These will fill the TSPs
//    double start_time = 0.0;
//    std::vector<double> values(1e5);
//    std::vector<double> time_double(1e5);
//    std::generate(time_double.begin(), time_double.end(), rand);
//    std::generate(values.begin(), values.end(), rand);
//
//    for (int i=0; i<100; i++)
//    {
//      std::cout << "Creating TSP # " << i << ". Memory used " << memory_usage() << " kb\n";
//      TimeSeriesProperty<double> * tsp = new  TimeSeriesProperty<double>("fake");
//      tsp->create(start_time, time_double, values);
//      Property * prop = dynamic_cast<Property *>(tsp);
//      delete prop;
//    }
//  }
//
//
//  // THIS DOES NOT LEAK after i=2
//  void xtestLeak3()
//  {
//    for (int i=0; i<4; i++)
//    {
//      std::cout << "Creating event workspace  call # " << i << ". Memory used " << memory_usage() << " kb\n";
//      int numPixels = 1000;
//      int numEvents = 10000;
//      EventWorkspace_sptr ew2 = WorkspaceCreationHelper::CreateEventWorkspace(numPixels, numEvents, numEvents);
//      //Overwrite
//      AnalysisDataService::Instance().addOrReplace("ew1", ew2);
//    }
//  }
//
//
//  // THIS DOES NOT LEAK after i=2
//  void xtestLeak4()
//  {
//    for (int i=0; i<50; i++)
//    {
//      std::cout << "CreateEventWorkspace call # " << i << ". Memory used " << memory_usage() << " kb\n";
//      IAlgorithm * ld = Mantid::API::FrameworkManager::Instance().createAlgorithm("CreateEventWorkspace");
//      //IAlgorithm_sptr ld = AlgorithmFactory::Instance().create("CreateEventWorkspace",1);
//      std::string outws_name = "fake";
//      ld->initialize();
//      ld->setPropertyValue("OutputWorkspace",outws_name);
//      ld->setPropertyValue("NumSpectra", "1000");
//      ld->setPropertyValue("NumEvents", "10000");
//      ld->execute();
//      TS_ASSERT( ld->isExecuted() );
//
//      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
//      //Valid WS and it is an EventWorkspace
//      TS_ASSERT( WS );
//      //Events
//      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1e7);
//    }
//  }
//
//
//  // THIS DOES LEAK !
//  void xtest_MemoryLeak_in_LoadLogsFromSNSNexus()
//  {
//    EventWorkspace_sptr ew2 = WorkspaceCreationHelper::CreateEventWorkspace(10, 20, 30);
//    AnalysisDataService::Instance().addOrReplace("fake", ew2);
//
//    std::cout << "About to start ... memory used " << memory_usage() << " kb\n";
//
//    for (int i=0; i<20; i++)
//    {
//      std::cout << "LoadLogsFromSNSNexus call # " << i << ". Memory used " << memory_usage() << " kb\n";
//      IAlgorithm_sptr ld = AlgorithmFactory::Instance().create("LoadLogsFromSNSNexus",1);
//      std::string outws_name = "cncs";
//      ld->initialize();
//      ld->setPropertyValue("Filename","PG3_1370_event.nxs");
//      ld->setPropertyValue("Workspace","fake");
//      ld->execute();
//      TS_ASSERT( ld->isExecuted() );
//    }
//  }
//
//  // THIS MIGHT LEAK ?
//  void xtest_MemoryLeak_inLoadSNSEventNexus()
//  {
//    std::cout << "About to start ... memory used " << memory_usage() << " kb\n";
//
//    for (int i=0; i<50; i++)
//    {
//      std::cout << "LoadSNSEventNexus call # " << i << ". Memory used " << memory_usage() << " kb\n";
//
//      std::string outws_name = "cncs";
//      AnalysisDataService::Instance().remove(outws_name);
//
//      LoadSNSEventNexus ld;
//
//      ld.initialize();

//      CNCS_7850_event.nxs should be replaced by CNCS_7860_event.nxs and check if you need to change anything else

//      ld.setPropertyValue("Filename","CNCS_7850_event.nxs");
//      ld.setPropertyValue("OutputWorkspace",outws_name);
//      ld.setPropertyValue("FilterByTof_Min", "-1e6");
//      ld.setPropertyValue("FilterByTof_Max", "1e6");
//      ld.setPropertyValue("FilterByTime_Start", "-1e6");
//      ld.setPropertyValue("FilterByTime_Stop", "1e6");
//
//      ld.execute();
//      TS_ASSERT( ld.isExecuted() );
//
//      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
//      //Valid WS and it is an EventWorkspace
//      TS_ASSERT( WS );
//      //Events
//      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
//    }
//    AnalysisDataService::Instance().remove("cncs");
//    std::cout << "Removed the last one ... memory used " << memory_usage() << " kb\n";
//  }
//
//  // ! THIS LEAKS MORE THAN THE PREVIOUS ONE !
//  void test_MemoryLeak_inLoadSNSEventNexus2()
//  {
//    for (int i=0; i<5; i++)
//    {
//      std::cout << "LoadSNSEventNexus call # " << i << ". Memory used " << memory_usage() << " kb\n";
//      IAlgorithm_sptr ld = AlgorithmFactory::Instance().create("LoadSNSEventNexus",1);
//      std::string outws_name = "cncs";
//      ld->initialize();

//      CNCS_7850_event.nxs should be replaced by CNCS_7860_event.nxs and check if you need to change anything else

//      ld->setPropertyValue("Filename","CNCS_7850_event.nxs");
//      ld->setPropertyValue("OutputWorkspace",outws_name);
//      ld->setPropertyValue("FilterByTof_Min", "-1e6");
//      ld->setPropertyValue("FilterByTof_Max", "1e6");
//      ld->setPropertyValue("FilterByTime_Start", "-1e6");
//      ld->setPropertyValue("FilterByTime_Stop", "1e6");
//
//      ld->execute();
//      TS_ASSERT( ld->isExecuted() );
//
//      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
//      //Valid WS and it is an EventWorkspace
//      TS_ASSERT( WS );
//      //Events
//      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
//    }
//  }
//
//  // ! THIS LEAKS TOO !
//  void xtest_MemoryLeak_inLoadSNSEventNexus3()
//  {
//    for (int i=0; i<50; i++)
//    {
//      std::cout << "LoadSNSEventNexus call # " << i << "\n";
//
//      //IAlgorithm * ld = AlgorithmFactory::Instance().create("LoadSNSEventNexus");
//      IAlgorithm * ld = Mantid::API::FrameworkManager::Instance().createAlgorithm("LoadSNSEventNexus");
//
//      std::string outws_name = "cncs";
//      ld->initialize();

//      CNCS_7850_event.nxs should be replaced by CNCS_7860_event.nxs and check if you need to change anything else

//      ld->setPropertyValue("Filename","CNCS_7850_event.nxs");
//      ld->setPropertyValue("OutputWorkspace",outws_name);
//      ld->setPropertyValue("FilterByTof_Min", "-1e6");
//      ld->setPropertyValue("FilterByTof_Max", "1e6");
//      ld->setPropertyValue("FilterByTime_Start", "-1e6");
//      ld->setPropertyValue("FilterByTime_Stop", "1e6");
//
//      ld->execute();
//      TS_ASSERT( ld->isExecuted() );
//
//      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
//      //Valid WS and it is an EventWorkspace
//      TS_ASSERT( WS );
//      //Events
//      TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
//
//    }
//  }

};


#endif /*LOADSNSEVENTNEXUSTEST_H_*/



