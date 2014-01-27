#ifndef LOADISISNEXUSTEST_H_
#define LOADISISNEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/LoadISISNexus.h"
#include "MantidDataHandling/LoadISISNexus2.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadISISNexusTest : public CxxTest::TestSuite
{
private:

  // Helper method to fetch the log property entry corresponding to period.
  Property* fetchPeriodLog(MatrixWorkspace_sptr workspace, int expectedPeriodNumber)
  {
    std::stringstream period_number_stream;
    period_number_stream << expectedPeriodNumber;
    std::string log_name = "period " + period_number_stream.str();
    Property* p= workspace->run().getLogData(log_name);
    return p;
  }

  // Helper method to fetch the log property entry corresponding to the current period.
  Property* fetchCurrentPeriodLog(MatrixWorkspace_sptr workspace)
  {
    Property* p= workspace->run().getLogData("current_period");
    return p;
  }

  // Helper method to check that the log data contains a specific period number entry.
  void checkPeriodLogData(MatrixWorkspace_sptr workspace, int expectedPeriodNumber)
  {
    Property* p = NULL; 
    TS_ASSERT_THROWS_NOTHING(p = fetchPeriodLog(workspace, expectedPeriodNumber));
    TS_ASSERT(p != NULL)
    TSM_ASSERT_THROWS("Shouldn't have a period less than the expected entry", fetchPeriodLog(workspace, expectedPeriodNumber-1), Mantid::Kernel::Exception::NotFoundError);
    TSM_ASSERT_THROWS("Shouldn't have a period greater than the expected entry", fetchPeriodLog(workspace, expectedPeriodNumber+1), Mantid::Kernel::Exception::NotFoundError);
    Mantid::Kernel::TimeSeriesProperty<bool>* period_property = dynamic_cast<Mantid::Kernel::TimeSeriesProperty<bool>*>(p);
    TS_ASSERT(period_property);
    // Check that the logs also contain a current_period property.
    Property* current_period_log = fetchCurrentPeriodLog(workspace);
    TS_ASSERT_EQUALS(expectedPeriodNumber, atoi(current_period_log->value().c_str()));

    // Check time series properties have been filtered by period
    p = NULL;
    TSM_ASSERT_THROWS_NOTHING("Cannot retrieve stheta log", p = workspace->run().getLogData("stheta"));
    auto stheta = dynamic_cast<FilteredTimeSeriesProperty<double>*>(p);
    TSM_ASSERT("stheta log has not been converted to a FilteredTimeSeries", stheta);
    TS_ASSERT(42 > stheta->size());

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
        TS_ASSERT_EQUALS(ws->getSpectrum(1234)->getSpectrumNo(), 1235 );
        TS_ASSERT(ws->getSpectrum(1234)->hasDetectorID(1235) );

        const std::vector< Property* >& logs = ws->run().getLogData();
        TS_ASSERT_EQUALS(logs.size(), 62);

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
        TS_ASSERT_EQUALS(blog->size(),6);

        TS_ASSERT_EQUALS(ws->sample().getName(),"PMMA_SAN25_1.5%_TRANS_150");
        
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
        // Check the multiperiod proton charge extraction
        const Run& run = ws1->run();
        ArrayProperty<double>* protonChargeProperty = dynamic_cast<ArrayProperty<double>* >( run.getLogData("proton_charge_by_period") );
        double chargeSum = 0;
        for(size_t i = 0; i < grpWs->size(); ++i)
        {
          chargeSum += protonChargeProperty->operator()()[i];
        }
        PropertyWithValue<double>* totalChargeProperty = dynamic_cast<PropertyWithValue<double>* >( run.getLogData("gd_prtn_chrg"));
        double totalCharge = atof(totalChargeProperty->value().c_str());
        TSM_ASSERT_DELTA("Something is badly wrong if the sum accross the periods does not correspond to the total charge.", totalCharge, chargeSum, 0.000001);
        AnalysisDataService::Instance().remove(wsName);
    }

    // Test the stub remnant of version 1 of this algorithm - that it can be run without setting any properties, and throws an exception.
    void testRemovedVersion1Throws()
    {
      LoadISISNexus v1;
      v1.setRethrows(true);
      TS_ASSERT_THROWS_NOTHING( v1.initialize() );
      TS_ASSERT_THROWS( v1.execute(), Exception::NotImplementedError)
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
