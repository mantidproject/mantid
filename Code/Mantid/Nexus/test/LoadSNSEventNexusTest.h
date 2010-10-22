#ifndef LOADSNSEVENTNEXUSTEST_H_
#define LOADSNSEVENTNEXUSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidAPI/WorkspaceGroup.h"
#include <iostream>
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


class LoadSNSEventNexusTest : public CxxTest::TestSuite
{
public:


  void testExec()
    {
        Mantid::API::FrameworkManager::Instance();
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
        //Pixels have to be padded
        TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
        //Events
        TS_ASSERT_EQUALS( WS->getNumberEvents(), 1208875);
        //TOF limits found. There is a pad of +-1 given around the actual TOF founds.
        TS_ASSERT_DELTA( (*WS->refX(0))[0],  44138.7, 0.05);
        TS_ASSERT_DELTA( (*WS->refX(0))[1],  60830.4, 0.05);

        //Check one event from one pixel - does it have a reasonable pulse time
        TS_ASSERT( WS->getEventListPtr(1000)->getEvents()[0].pulseTime() > 1e9*365*10 );

        //Longer, more thorough test
        if (false)
        {
          IAlgorithm_sptr load =  AlgorithmManager::Instance().create("LoadEventPreNeXus", 1);
          load->setPropertyValue("OutputWorkspace", "cncs_pre");
          load->setPropertyValue("EventFilename","../../../../Test/AutoTestData/CNCS_7850_neutron_event.dat");
          load->setPropertyValue("PulseidFilename","../../../../Test/AutoTestData/CNCS_7850_pulseid.dat");
          load->setPropertyValue("MappingFilename","../../../../Test/AutoTestData/CNCS_TS_2008_08_18.dat");
          load->setPropertyValue("PadEmptyPixels","1");
          load->execute();
          TS_ASSERT( load->isExecuted() );
          DataObjects::EventWorkspace_sptr WS2 = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve("cncs_pre"));
          //Valid WS and it is an EventWorkspace
          TS_ASSERT( WS2 );

          //Let's compare the proton_charge logs
          Kernel::TimeSeriesProperty<double> * log = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( WS->mutableRun().getProperty("proton_charge") );
          std::map<dateAndTime, double> logMap = log->valueAsCorrectMap();
          Kernel::TimeSeriesProperty<double> * log2 = dynamic_cast<Kernel::TimeSeriesProperty<double> *>( WS2->mutableRun().getProperty("proton_charge") );
          std::map<dateAndTime, double> logMap2 = log2->valueAsCorrectMap();
          std::map<dateAndTime, double>::iterator it, it2;

          it = logMap.begin();
          it2 = logMap2.begin();
          Kernel::PulseTimeType start = DateAndTime::get_from_absolute_time(it->first);
          for (; it != logMap.end(); )
          {
            //std::cout << DateAndTime::to_simple_string( it->first) << " --- " << DateAndTime::to_simple_string( it2->first) <<  "\n";
            //Same times within a millisecond
            //TS_ASSERT_DELTA( it->first, it2->first, DateAndTime::duration_from_seconds(1e-3));
            //Same times?
            TS_ASSERT_LESS_THAN( fabs(DateAndTime::durationInSeconds(it->first - it2->first)), 1); //TODO: Fix the nexus file times here
            //Same proton charge?
            TS_ASSERT_DELTA( it->second, it2->second, 1e-5);
            it++;
            it2++;
          }

          int pixelID = 2000;

          std::vector<TofEvent> events1 = WS->getEventListPtr(pixelID)->getEvents();
          std::vector<TofEvent> events2 = WS2->getEventListPtr(pixelID)->getEvents();

          //std::cout << events1.size() << std::endl;
          TS_ASSERT_EQUALS( events1.size(), events2.size());
          if (events1.size() == events2.size())
          {
            for (int i=0; i < events1.size(); i++)
            {
              TS_ASSERT_DELTA( events1[i].tof(), events2[i].tof(), 0.05);
              TS_ASSERT_DELTA( events1[i].pulseTime(), events2[i].pulseTime(), 1e9); //TODO:: Fix nexus start times
              //std::cout << (events1[i].pulseTime()-start)/1e9 << " - " << (events2[i].pulseTime()-start)/1e9 << " sec\n";
              //std::cout << "Pulse time diff " << (events1[i].pulseTime() - events2[i].pulseTime())/1e9 << " sec\n";
            }
          }

        }


    }

    void testExec_Filtered()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadSNSEventNexus ld;
        std::string outws_name = "cncs";
        ld.initialize();
        ld.setPropertyValue("OutputWorkspace",outws_name);
        //ld.setPropertyValue("Filename","/home/janik/data/PG3_732_event.nxs");
        ld.setPropertyValue("Filename","../../../../Test/AutoTestData/CNCS_7850_event.nxs");
        ld.setPropertyValue("FilterByTime_Start", "300.0");
        ld.setPropertyValue("FilterByTime_Stop", "600.0");
        ld.setPropertyValue("FilterByTof_Min", "-1e10");
        ld.setPropertyValue("FilterByTof_Max", "1e10");

        ld.execute();
        TS_ASSERT( ld.isExecuted() );

        DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
        //Valid WS and it is an EventWorkspace
        TS_ASSERT( WS );
        //Pixels have to be padded
        TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
        //Events
        TS_ASSERT_EQUALS( WS->getNumberEvents(), 83774);

        //Check one event from one pixel - does it have a reasonable pulse time
        TS_ASSERT( WS->getEventListPtr(7)->getEvents()[0].pulseTime() > 1e9*365*10 );
    }


//
//    void xtestExec_SNAP()
//    {
//      Mantid::API::FrameworkManager::Instance();
//      LoadSNSEventNexus ld;
//      std::string outws_name = "snap";
//      ld.initialize();
//      ld.setPropertyValue("Filename","/home/8oz/data/SNAP_4105_event.nxs");
//      ld.setPropertyValue("OutputWorkspace",outws_name);
//      ld.setPropertyValue("FilterByTof_Min", "-1e10");
//      ld.setPropertyValue("FilterByTof_Max", "1e10");
//      ld.setPropertyValue("FilterByTime_Start", "-1e10");
//      ld.setPropertyValue("FilterByTime_Stop", "1e10");
//
//      ld.execute();
//      TS_ASSERT( ld.isExecuted() );
//
//      DataObjects::EventWorkspace_sptr WS = boost::dynamic_pointer_cast<DataObjects::EventWorkspace>(AnalysisDataService::Instance().retrieve(outws_name));
//      //Valid WS and it is an EventWorkspace
//      TS_ASSERT( WS );
//      //Pixels have to be padded
//      TS_ASSERT_EQUALS( WS->getNumberHistograms(), 65536 * 18);
//
//    }


};


#endif /*LOADSNSEVENTNEXUSTEST_H_*/



