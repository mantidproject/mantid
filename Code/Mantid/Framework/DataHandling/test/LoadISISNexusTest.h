#ifndef LOADISISNEXUSTEST_H_
#define LOADISISNEXUSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/IDTypes.h"

#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/LogFilter.h"
#include "MantidKernel/FilteredTimeSeriesProperty.h"

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/WorkspaceGroup.h"

#include "MantidDataHandling/LoadISISNexus.h"
#include "MantidDataHandling/LoadISISNexus2.h"

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

  void testExecMonSeparated()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename","LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace","outWS");
    ld.setPropertyValue("LoadMonitors","1"); // should read "Separate"
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());


    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    MatrixWorkspace_sptr mon_ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");

    TS_ASSERT_EQUALS(ws->blocksize(),5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(),17790);

    TS_ASSERT_EQUALS(mon_ws->blocksize(),5);
    TS_ASSERT_EQUALS(mon_ws->getNumberHistograms(),2);

    // Two monitors which form two first spectra are excluded by load separately

    // spectrum with ID 5 is now spectrum N 3 as 2 monitors
    TS_ASSERT_EQUALS(ws->readY(5-2)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5-2)->getSpectrumNo(),6);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5-2)->getDetectorIDs().begin()), 6);
    // spectrum with ID 7 is now spectrum N 4
    TS_ASSERT_EQUALS(ws->readY(6-2)[0],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6-2)->getSpectrumNo(),7);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6-2)->getDetectorIDs().begin()), 7);
    //
    TS_ASSERT_EQUALS(ws->readY(8-2)[3],1.);

    TS_ASSERT_EQUALS(mon_ws->readX(0)[0],5.);
    TS_ASSERT_EQUALS(mon_ws->readX(0)[1],4005.);
    TS_ASSERT_EQUALS(mon_ws->readX(0)[2],8005.);

    // these spectra are not loaded as above so their values are different  (occasionally 0)
    TS_ASSERT_EQUALS(mon_ws->readY(0)[1],0);
    TS_ASSERT_EQUALS(mon_ws->readY(1)[0],0.);
    TS_ASSERT_EQUALS(mon_ws->readY(0)[3],0.);


  
    const std::vector< Property* >& logs = mon_ws->run().getLogData();
    TS_ASSERT_EQUALS(logs.size(), 62);

    std::string header = mon_ws->run().getPropertyValueAsType<std::string>("run_header");
    TS_ASSERT_EQUALS(86, header.size());
    TS_ASSERT_EQUALS("LOQ 49886 Team LOQ             Quiet Count, ISIS Off, N 28-APR-2009  09:20:29     0.00", header);

    TimeSeriesProperty<std::string>* slog = dynamic_cast<TimeSeriesProperty<std::string>*>(mon_ws->run().getLogData("icp_event"));
    TS_ASSERT(slog);
    std::string str = slog->value();
    TS_ASSERT_EQUALS(str.size(),1023);
    TS_ASSERT_EQUALS(str.substr(0,37),"2009-Apr-28 09:20:29  CHANGE_PERIOD 1");

    slog = dynamic_cast<TimeSeriesProperty<std::string>*>(mon_ws->run().getLogData("icp_debug"));
    TS_ASSERT(slog);
    TS_ASSERT_EQUALS(slog->size(),50);

    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
  }





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
    TS_ASSERT_EQUALS(ws->getSpectrum(0)->getSpectrumNo(),1);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(0)->getDetectorIDs().begin()), 1);


    TS_ASSERT_EQUALS(ws->readY(5)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5)->getSpectrumNo(),6);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5)->getDetectorIDs().begin()), 6);
    TS_ASSERT_EQUALS(ws->readY(6)[0],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6)->getSpectrumNo(),7);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6)->getDetectorIDs().begin()), 7);
    TS_ASSERT_EQUALS(ws->readY(8)[3],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(8)->getSpectrumNo(),9);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(8)->getDetectorIDs().begin()), 9);

    TS_ASSERT_EQUALS(ws->readY(13)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13)->getSpectrumNo(),14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(13)->getDetectorIDs().begin()), 14);
    TS_ASSERT_EQUALS(ws->readY(17)[1],2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17)->getSpectrumNo(),18);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(17)->getDetectorIDs().begin()), 18);
    TS_ASSERT_EQUALS(ws->readY(18)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18)->getSpectrumNo(),19);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(18)->getDetectorIDs().begin()), 19);


    TS_ASSERT_EQUALS(ws->readY(33)[2],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(33)->getSpectrumNo(),34);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(33)->getDetectorIDs().begin()), 34);
    TS_ASSERT_EQUALS(ws->readY(34)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(34)->getSpectrumNo(),35);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(34)->getDetectorIDs().begin()), 35);

    TS_ASSERT_EQUALS(ws->readY(37)[3],1.);
    TS_ASSERT_EQUALS(ws->readY(37)[4],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(37)->getSpectrumNo(),38);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(37)->getDetectorIDs().begin()), 38);


    TS_ASSERT_EQUALS(ws->getSpectrum(1234)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(1234)->getDetectorIDs().begin()), 1235);

    TS_ASSERT_EQUALS(ws->getSpectrum(1234)->getSpectrumNo(), 1235 );
    TS_ASSERT(ws->getSpectrum(1234)->hasDetectorID(1235) );

    const std::vector< Property* >& logs = ws->run().getLogData();
    TS_ASSERT_EQUALS(logs.size(), 62);

    std::string header = ws->run().getPropertyValueAsType<std::string>("run_header");
    TS_ASSERT_EQUALS(86, header.size());
    TS_ASSERT_EQUALS("LOQ 49886 Team LOQ             Quiet Count, ISIS Off, N 28-APR-2009  09:20:29     0.00", header);

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
    ld.setPropertyValue("SpectrumList","5,34,35,38");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(),5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(),15);

    TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
    TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
    TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);
    TS_ASSERT_EQUALS(ws->getSpectrum(0)->getSpectrumNo(),5);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(0)->getDetectorIDs().begin()), 5);

    // these spectra are not loaded as above so their values are different (occasionally 0)
    TSM_ASSERT_EQUALS("Total workspace spectra N13, index 1 is occasionally 1 ",ws->readY(5)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5)->getSpectrumNo(),14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5)->getDetectorIDs().begin()),14);
    TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6)->getSpectrumNo(),15);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6)->getDetectorIDs().begin()),15);
    TS_ASSERT_EQUALS(ws->readY(8)[3],0.);
    TS_ASSERT_EQUALS(ws->getSpectrum(8)->getSpectrumNo(),17);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(8)->getDetectorIDs().begin()),17);


    // look at the same values as the full loader above
    TS_ASSERT_EQUALS(ws->readY(13-8)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13-8)->getSpectrumNo(),14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(13-8)->getDetectorIDs().begin()),14);

    TS_ASSERT_EQUALS(ws->readY(17-8)[1],2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17-8)->getSpectrumNo(),18);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(17-8)->getDetectorIDs().begin()),18);
    TS_ASSERT_EQUALS(ws->readY(18-8)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18-8)->getSpectrumNo(),19);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(18-8)->getDetectorIDs().begin()),19);

    // look at the same values as the full loader above
    TS_ASSERT_EQUALS(ws->readY(33-21)[2],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(33-21)->getSpectrumNo(),34);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(33-21)->getDetectorIDs().begin()),34);
    TS_ASSERT_EQUALS(ws->readY(34-21)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(34-21)->getSpectrumNo(),35);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(34-21)->getDetectorIDs().begin()),35);
    TS_ASSERT_EQUALS(ws->readY(37-23)[3],1.);
    TS_ASSERT_EQUALS(ws->readY(37-23)[4],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(37-23)->getSpectrumNo(),38 );
    TS_ASSERT_EQUALS(*(ws->getSpectrum(37-23)->getDetectorIDs().begin()),38);



    TS_ASSERT_EQUALS(ws->getSpectrum(0)->getSpectrumNo(), 5 );
    TS_ASSERT_EQUALS(*(ws->getSpectrum(0)->getDetectorIDs().begin()), 5);
    TS_ASSERT(ws->getSpectrum(0)->hasDetectorID(5) );

    TS_ASSERT_EQUALS(ws->getSpectrum(1)->getSpectrumNo(), 10 );
    TS_ASSERT_EQUALS(*(ws->getSpectrum(1)->getDetectorIDs().begin()), 10);
    TS_ASSERT(ws->getSpectrum(1)->hasDetectorID(10) );

    std::vector<size_t> spectNum2WSInd;
    Mantid::specid_t  offset;
    ws->getSpectrumToWorkspaceIndexVector(spectNum2WSInd,offset);
    TS_ASSERT_EQUALS(38+offset+1,spectNum2WSInd.size());
    size_t sample[]={5,10,11,12,13,14,15,16,17,18,19,20,34,35,38};
    std::vector<size_t> Sample(sample,sample+15);
    for(size_t i=0;i<Sample.size();i++)
    {
      TS_ASSERT_EQUALS(i,spectNum2WSInd[Sample[i]+offset]);
    }

    TS_ASSERT_EQUALS(ws->getSpectrum(14)->getSpectrumNo(), 38 );
    TS_ASSERT_EQUALS(*(ws->getSpectrum(14)->getDetectorIDs().begin()), 38);
    TS_ASSERT(ws->getSpectrum(14)->hasDetectorID(38) );


    AnalysisDataService::Instance().remove("outWS");
  }
  void testExec3()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename","LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace","outWS");
    ld.setPropertyValue("SpectrumMin","10");
    ld.setPropertyValue("SpectrumMax","20");
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());

    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(),5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(),11);

    TS_ASSERT_EQUALS(ws->readX(0)[0],5.);
    TS_ASSERT_EQUALS(ws->readX(0)[1],4005.);
    TS_ASSERT_EQUALS(ws->readX(0)[2],8005.);

    // these spectra are not loaded as above so their values are different  (occasionally 0)
    TS_ASSERT_EQUALS(ws->readY(5)[1],0);
    TS_ASSERT_EQUALS(ws->readY(6)[0],0.);
    TS_ASSERT_EQUALS(ws->readY(8)[3],0.);

    // look at the same values as the full/partial loader above
    TS_ASSERT_EQUALS(ws->readY(13-9)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13-9)->getSpectrumNo(),14);
    TS_ASSERT_EQUALS(ws->readY(17-9)[1],2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17-9)->getSpectrumNo(),18);
    TS_ASSERT_EQUALS(ws->readY(18-9)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18-9)->getSpectrumNo(),19);

    std::vector<size_t> spectNum2WSInd;
    Mantid::specid_t  offset;
    ws->getSpectrumToWorkspaceIndexVector(spectNum2WSInd,offset);
    TS_ASSERT_EQUALS(20+offset+1,spectNum2WSInd.size());
    size_t sample[]={10,11,12,13,14,15,16,17,18,19,20};
    std::vector<size_t> Sample(sample,sample+10);
    for(size_t i=0;i<Sample.size();i++)
    {
      TS_ASSERT_EQUALS(i,spectNum2WSInd[Sample[i]+offset]);
    }

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

    TS_ASSERT_EQUALS(ws->readY(5)[1],0);
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
    // Check the multi-period proton charge extraction
    const Run& run = ws1->run();
    ArrayProperty<double>* protonChargeProperty = dynamic_cast<ArrayProperty<double>* >( run.getLogData("proton_charge_by_period") );
    double chargeSum = 0;
    for(size_t i = 0; i < grpWs->size(); ++i)
    {
      chargeSum += protonChargeProperty->operator()()[i];
    }
    PropertyWithValue<double>* totalChargeProperty = dynamic_cast<PropertyWithValue<double>* >( run.getLogData("gd_prtn_chrg"));
    double totalCharge = atof(totalChargeProperty->value().c_str());
    TSM_ASSERT_DELTA("Something is badly wrong if the sum across the periods does not correspond to the total charge.", totalCharge, chargeSum, 0.000001);
    AnalysisDataService::Instance().remove(wsName);
  }

  void test_instrument_and_default_param_loaded_when_inst_not_in_nexus_file()
  {
    Mantid::API::FrameworkManager::Instance();
    const std::string wsName = "InstNotInNexus";
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
    MatrixWorkspace_sptr ws1 = boost::dynamic_pointer_cast<MatrixWorkspace>(grpWs->getItem(0));

    auto inst = ws1->getInstrument();
    TS_ASSERT( !inst->getFilename().empty()); // This is how we know we didn't get it from inside the nexus file
    TS_ASSERT_EQUALS( inst->getName(), "POLREF" );
    TS_ASSERT_EQUALS( inst->getNumberDetectors(), 885 );

    // check that POLREF_Parameters.xml has been loaded
    auto params = inst->getParameterMap();
    TS_ASSERT_EQUALS( params->getString(inst.get(), "show-signed-theta"), "Always");
  }


  // Test the stub remnant of version 1 of this algorithm - that it can be run without setting any properties, and throws an exception.
  void testRemovedVersion1Throws()
  {
    LoadISISNexus v1;
    v1.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING( v1.initialize() );
    TS_ASSERT_THROWS( v1.execute(), Exception::NotImplementedError)
  }
  void testExecMonExcluded()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename","LOQ49886.nxs");
    ld.setPropertyValue("OutputWorkspace","outWS");
    ld.setPropertyValue("LoadMonitors","0"); // should read "exclude"
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());


    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(),5);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(),17790);

    // Two monitors which form two first spectra are excluded by load separately

    // spectrum with ID 5 is now spectrum N 3 as 2 monitors
    TS_ASSERT_EQUALS(ws->readY(5-2)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(5-2)->getSpectrumNo(),6);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(5-2)->getDetectorIDs().begin()), 6);
    // spectrum with ID 7 is now spectrum N 4
    TS_ASSERT_EQUALS(ws->readY(6-2)[0],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(6-2)->getSpectrumNo(),7);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(6-2)->getDetectorIDs().begin()), 7);
    //
    TS_ASSERT_EQUALS(ws->readY(8-2)[3],1.);

    // spectrum with ID 9 is now spectrum N 6
    TS_ASSERT_EQUALS(ws->getSpectrum(8-2)->getSpectrumNo(),9);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(8-2)->getDetectorIDs().begin()), 9);
    // spectrum with ID 14 is now spectrum N 11
    TS_ASSERT_EQUALS(ws->readY(13-2)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(13-2)->getSpectrumNo(),14);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(13-2)->getDetectorIDs().begin()), 14);
    // spectrum with ID 18 is now spectrum N 15
    TS_ASSERT_EQUALS(ws->readY(17-2)[1],2.);
    TS_ASSERT_EQUALS(ws->getSpectrum(17-2)->getSpectrumNo(),18);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(17-2)->getDetectorIDs().begin()), 18);
    // spectrum with ID 19 is now spectrum N 16
    TS_ASSERT_EQUALS(ws->readY(18-2)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(18-2)->getSpectrumNo(),19);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(18-2)->getDetectorIDs().begin()), 19);


    TS_ASSERT_EQUALS(ws->readY(33-2)[2],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(33-2)->getSpectrumNo(),34);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(33-2)->getDetectorIDs().begin()), 34);
    //
    TS_ASSERT_EQUALS(ws->readY(34-2)[1],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(34-2)->getSpectrumNo(),35);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(34-2)->getDetectorIDs().begin()), 35);

    TS_ASSERT_EQUALS(ws->readY(37-2)[3],1.);
    TS_ASSERT_EQUALS(ws->readY(37-2)[4],1.);
    TS_ASSERT_EQUALS(ws->getSpectrum(37-2)->getSpectrumNo(),38);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(37-2)->getDetectorIDs().begin()), 38);


    TS_ASSERT_EQUALS(ws->getSpectrum(1234-2)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS(*(ws->getSpectrum(1234-2)->getDetectorIDs().begin()), 1235);

    TS_ASSERT_EQUALS(ws->getSpectrum(1234-2)->getSpectrumNo(), 1235 );
    TS_ASSERT(ws->getSpectrum(1234-2)->hasDetectorID(1235) );

    const std::vector< Property* >& logs = ws->run().getLogData();
    TS_ASSERT_EQUALS(logs.size(), 62);

    std::string header = ws->run().getPropertyValueAsType<std::string>("run_header");
    TS_ASSERT_EQUALS(86, header.size());
    TS_ASSERT_EQUALS("LOQ 49886 Team LOQ             Quiet Count, ISIS Off, N 28-APR-2009  09:20:29     0.00", header);

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

  void xestExecMonExcludedInTheEnd()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadISISNexus2 ld;
    ld.initialize();
    ld.setPropertyValue("Filename","MAPS00018314.nxs");
    ld.setPropertyValue("SpectrumMin","2");
    ld.setPropertyValue("SpectrumMax","10");
    ld.setPropertyValue("OutputWorkspace","outWS");
    ld.setPropertyValue("LoadMonitors","Separate"); //
    TS_ASSERT_THROWS_NOTHING(ld.execute());
    TS_ASSERT(ld.isExecuted());


    MatrixWorkspace_sptr ws = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    TS_ASSERT_EQUALS(ws->blocksize(),2000);
    TS_ASSERT_EQUALS(ws->getNumberHistograms(),9);

    MatrixWorkspace_sptr ws_mon = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS_monitors");
    TS_ASSERT(ws_mon)

    TS_ASSERT_EQUALS(ws_mon->blocksize(),2000);
    TS_ASSERT_EQUALS(ws_mon->getNumberHistograms(),4);
    TS_ASSERT_DELTA(ws_mon->readX(0)[0],10,1.e-8);


    TS_ASSERT_EQUALS(ws_mon->getSpectrum(0)->getSpectrumNo(),41473);
    TS_ASSERT_EQUALS(ws_mon->getSpectrum(3)->getSpectrumNo(),41476);


    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS_monitors");
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
