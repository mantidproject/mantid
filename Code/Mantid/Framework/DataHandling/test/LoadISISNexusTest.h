#ifndef LOADISISNEXUSTEST_H_
#define LOADISISNEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadISISNexus.h"
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectraDetectorMap.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/LogFilter.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadISISNexusTest : public CxxTest::TestSuite
{
private:

  // Helper method to fetch the log property entry corresponding to period.
  Property* fetchPeriod(MatrixWorkspace_sptr workspace, int expectedPeriodNumber)
  {
    std::stringstream period_number_stream;
    period_number_stream << expectedPeriodNumber;
    std::string log_name = "period " + period_number_stream.str();
    Property* p= workspace->run().getLogData(log_name);
    return p;
  }

  // Helper method to check that the log data contains a specific period number entry.
  void checkPeriodLogData(MatrixWorkspace_sptr workspace, int expectedPeriodNumber)
  {
    Property* p = NULL; 
    TS_ASSERT_THROWS_NOTHING(p = fetchPeriod(workspace, expectedPeriodNumber));
    TS_ASSERT(p != NULL)
    TSM_ASSERT_THROWS("Shouldn't have a period less than the expected entry", fetchPeriod(workspace, expectedPeriodNumber-1), Mantid::Kernel::Exception::NotFoundError);
    TSM_ASSERT_THROWS("Shouldn't have a period greater than the expected entry", fetchPeriod(workspace, expectedPeriodNumber+1), Mantid::Kernel::Exception::NotFoundError);
    Mantid::Kernel::TimeSeriesProperty<bool>* period_property = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool>*>(p);
    TS_ASSERT(period_property);
  }

public:
    void testExec()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","LOQ49886.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

        MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
        TS_ASSERT_EQUALS(ws->blocksize(),5);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),17792);
        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],1.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],1.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],1.);

        TS_ASSERT_EQUALS(ws->getSpectrum(1234)->getDetectorIDs().size(), 1);

        const std::vector< Property* >& logs = ws->run().getLogData();
        TS_ASSERT_EQUALS(logs.size(), 60);

        TimeSeriesProperty<std::string>* slog = dynamic_cast<TimeSeriesProperty<std::string>*>(ws->run().getLogData("icp_event"));
        TS_ASSERT(slog);
        std::string str = slog->value();
        TS_ASSERT_EQUALS(str.size(),1023);
        TS_ASSERT_EQUALS(str.substr(0,37),"2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

        slog = dynamic_cast<TimeSeriesProperty<std::string>*>(ws->run().getLogData("icp_debug"));
        TS_ASSERT(slog);
        TS_ASSERT_EQUALS(slog->size(),50);

        TimeSeriesProperty<int>* ilog = dynamic_cast<TimeSeriesProperty<int>*>(ws->run().getLogData("total_counts"));
        TS_ASSERT(ilog);
        TS_ASSERT_EQUALS(ilog->size(),172);

        ilog = dynamic_cast<TimeSeriesProperty<int>*>(ws->run().getLogData("period"));
        TS_ASSERT(ilog);
        TS_ASSERT_EQUALS(ilog->size(),172);

        TimeSeriesProperty<double> *dlog = dynamic_cast<TimeSeriesProperty<double>*>(ws->run().getLogData("proton_charge"));
        TS_ASSERT(dlog);
        TS_ASSERT_EQUALS(dlog->size(),172);


        TimeSeriesProperty<bool>* blog = dynamic_cast<TimeSeriesProperty<bool>*>(ws->run().getLogData("period 1"));
        TS_ASSERT(blog);
        TS_ASSERT_EQUALS(blog->size(),1);

        blog = dynamic_cast<TimeSeriesProperty<bool>*>(ws->run().getLogData("running"));
        TS_ASSERT(blog);
        TS_ASSERT_EQUALS(blog->size(),2);

        TS_ASSERT_EQUALS(ws->sample().getName(),"");
        
        Property *l_property = ws->run().getLogData( "run_number" );
        TS_ASSERT_EQUALS( l_property->value(), "49886" );
        AnalysisDataService::Instance().remove("outWS");
    }
    void testExec2()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","LOQ49886.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
        ld.setPropertyValue("SpectrumList","30,33,38");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());

            MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
        TS_ASSERT_EQUALS(ws->blocksize(),5);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),14);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(8)[1],2.);
        TS_ASSERT_EQUALS(ws->readY(10)[3],1.);
        TS_ASSERT_EQUALS(ws->readY(13)[4],1.);
        AnalysisDataService::Instance().remove("outWS");
    }
         void testMultiPeriodEntryNumberZero()
    {
                Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","TEST00000008.nxs");
            ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","19");
                ld.setPropertyValue("EntryNumber","0");
                //ld.setPropertyValue("SpectrumList","30,31");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());
                
                WorkspaceGroup_sptr grpout;//=WorkspaceGroup_sptr(new WorkspaceGroup);
                TS_ASSERT_THROWS_NOTHING(grpout=AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("outWS"));

        MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_1");
        TS_ASSERT_EQUALS(ws->blocksize(),995);
        TS_ASSERT_EQUALS(ws->getNumberHistograms(),10);
        TS_ASSERT_DELTA(ws->run().getProtonCharge(), 0.069991, 1e-6);

        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],6.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],7.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[1],0.);
        AnalysisDataService::Instance().remove("outWS");
    }
          void testMultiPeriodEntryNumberNonZero()
    {
        Mantid::API::FrameworkManager::Instance();
        LoadISISNexus2 ld;
        ld.initialize();
        ld.setPropertyValue("Filename","TEST00000008.nxs");
        ld.setPropertyValue("OutputWorkspace","outWS");
        ld.setPropertyValue("SpectrumMin","10");
        ld.setPropertyValue("SpectrumMax","20");
        //      ld.setPropertyValue("SpectrumList","29,30,31");
                ld.setPropertyValue("EntryNumber","5");
        TS_ASSERT_THROWS_NOTHING(ld.execute());
        TS_ASSERT(ld.isExecuted());
                        

        MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
        TS_ASSERT_EQUALS(ws->blocksize(),995);
     //   TS_ASSERT_EQUALS(ws->getNumberHistograms(),14);
        TS_ASSERT_EQUALS(ws->getTitle(), "hello\\0");
        TS_ASSERT_DELTA(ws->run().getProtonCharge(), 0.069991, 1e-6);
        TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
        TS_ASSERT_EQUALS(ws->readX(0)[1],6.);
        TS_ASSERT_EQUALS(ws->readX(0)[2],7.);
        
        TS_ASSERT_EQUALS(ws->readY(5)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
        TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

        TS_ASSERT_EQUALS(ws->readY(7)[1],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[3],0.);
        TS_ASSERT_EQUALS(ws->readY(9)[1],0.);
        AnalysisDataService::Instance().remove("outWS");
    }

    void testLoadMultiPeriodData()
    {
        Mantid::API::FrameworkManager::Instance();
        const std::string wsName = "outWS";
        LoadISISNexus2 loadingAlg;
        loadingAlg.initialize();
        loadingAlg.setRethrows(true);
        loadingAlg.setPropertyValue("Filename","POLREF00004699.nxs");
        loadingAlg.setPropertyValue("OutputWorkspace", wsName);
        loadingAlg.execute();
        TS_ASSERT(loadingAlg.isExecuted());

        AnalysisDataServiceImpl& ADS = AnalysisDataService::Instance();

        WorkspaceGroup_sptr grpWs;
        TS_ASSERT_THROWS_NOTHING(grpWs=ADS.retrieveWS<WorkspaceGroup>(wsName));
        TSM_ASSERT_EQUALS("Should be two workspaces in the group",2, grpWs->size());

        // Check the individual workspace group members.
        MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(grpWs->getItem(0));
        MatrixWorkspace_sptr ws2 = boost::dynamic_pointer_cast<MatrixWorkspace>(grpWs->getItem(1));
        TS_ASSERT(ws1 != NULL);
        TS_ASSERT(ws2 != NULL);
        // Check that workspace 1 has the correct period data, and no other period log data
        checkPeriodLogData(ws1, 1);
        // Check that workspace 2 has the correct period data, and no other period log data
        checkPeriodLogData(ws2, 2);
        AnalysisDataService::Instance().remove(wsName);
    }
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadISISNexusTestPerformance : public CxxTest::TestSuite
{
public:
  void testDefaultLoad()
  {
    LoadISISNexus2 loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "LOQ49886.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT( loader.execute() );
  }
};

#endif /*LOADISISNEXUSTEST_H_*/
