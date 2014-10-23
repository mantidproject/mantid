#ifndef LOADEVENTNEXUSTEST_H_
#define LOADEVENTNEXUSTEST_H_

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Workspace.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/WorkspaceOpOverloads.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidKernel/PhysicalConstants.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Timer.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include <cxxtest/TestSuite.h>
#include <iostream>
#include <Poco/File.h>

using namespace Mantid::Geometry;
using namespace Mantid::API;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;

class LoadEventNexusTest : public CxxTest::TestSuite
{
private:

  void do_test_filtering_start_and_end_filtered_loading(const bool metadataonly)
  {
    const std::string wsName = "test_filtering";
    const double filterStart = 1;
    const double filterEnd = 1000;

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setProperty("FilterByTimeStart", filterStart);
    ld.setProperty("FilterByTimeStop", filterEnd);
    ld.setProperty("MetaDataOnly", metadataonly);

    TS_ASSERT( ld.execute() );

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName); 

    Property* prop = outWs->run().getLogData("SampleTemp");
    TSM_ASSERT_EQUALS("Should have 16 elements after filtering.", 16, prop->size());
    if(prop->size() != 16)
      return;
    //Further tests
    TimeSeriesProperty<double>* sampleTemps = dynamic_cast<TimeSeriesProperty<double>* >(prop);
    auto filteredLogStartTime = sampleTemps->nthTime(0);
    auto filteredLogEndTime = sampleTemps->nthTime(sampleTemps->size()-1);
    TS_ASSERT_EQUALS("2010-Mar-25 16:09:27.620000000",filteredLogStartTime.toSimpleString());
    TS_ASSERT_EQUALS("2010-Mar-25 16:11:51.558003540",filteredLogEndTime.toSimpleString());
  }

public:

  void test_SingleBank_PixelsOnlyInThatBank()
  {
    doTestSingleBank(true, false);
  }

  void test_Normal_vs_Precount()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs_noprecount";
    ld.initialize();
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace",outws_name);
    ld.setPropertyValue("Precount", "0");
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name) );
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS );
    //Pixels have to be padded
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
    //Events
    TS_ASSERT_EQUALS( WS->getNumberEvents(), 112266);
    //TOF limits found. There is a pad of +-1 given around the actual TOF founds.
    TS_ASSERT_DELTA( (*WS->refX(0))[0],  44162.6, 0.05);
    TS_ASSERT_DELTA( (*WS->refX(0))[1],  60830.2, 0.05);
    // Valid spectrum info
    TS_ASSERT_EQUALS( WS->getSpectrum(0)->getSpectrumNo(), 1);
    TS_ASSERT_EQUALS( WS->getSpectrum(0)->getDetectorIDs().size(), 1);
    TS_ASSERT_EQUALS( *WS->getSpectrum(0)->getDetectorIDs().begin(), 0);

    //Check one event from one pixel - does it have a reasonable pulse time
    TS_ASSERT( WS->getEventListPtr(1000)->getEvents()[0].pulseTime() > DateAndTime(int64_t(1e9*365*10)) );

    //Check filename
    TS_ASSERT_EQUALS(ld.getPropertyValue("Filename"),WS->run().getProperty("Filename")->value());

    // Test that asking not to load the logs did what it should
    // Make sure that we throw if we try to read a log (that shouldn't be there)
    TS_ASSERT_THROWS( WS->getLog("proton_charge"),  std::invalid_argument);

    //----- Now we re-load with precounting and compare memory use ----
    LoadEventNexus ld2;
    std::string outws_name2 = "cncs_precount";
    ld2.initialize();
    ld2.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld2.setPropertyValue("OutputWorkspace",outws_name2);
    ld2.setPropertyValue("Precount", "1");
    ld2.setProperty<bool>("LoadLogs", false); // Time-saver
    ld2.execute();
    TS_ASSERT( ld2.isExecuted() );

    EventWorkspace_sptr WS2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name2);
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS2 );

    TS_ASSERT_EQUALS( WS->getNumberEvents(), WS2->getNumberEvents() );
    // Memory used should be lower (or the same at worst)
    TS_ASSERT_LESS_THAN_EQUALS( WS2->getMemorySize(), WS->getMemorySize() );



    //Longer, more thorough test
    if (false)
    {
      IAlgorithm_sptr load =  AlgorithmManager::Instance().create("LoadEventPreNexus", 1);
      load->setPropertyValue("OutputWorkspace", "cncs_pre");
      load->setPropertyValue("EventFilename","CNCS_7860_neutron_event.dat");
      load->setPropertyValue("PulseidFilename","CNCS_7860_pulseid.dat");
      load->setPropertyValue("MappingFilename","CNCS_TS_2008_08_18.dat");
      load->execute();
      TS_ASSERT( load->isExecuted() );
      EventWorkspace_sptr WS2 = AnalysisDataService::Instance().retrieveWS<EventWorkspace>("cncs_pre");
      //Valid WS and it is an EventWorkspace
      TS_ASSERT( WS2 );

      //Let's compare the proton_charge logs
      TimeSeriesProperty<double> * log = dynamic_cast<TimeSeriesProperty<double> *>( WS->mutableRun().getProperty("proton_charge") );
      std::map<DateAndTime, double> logMap = log->valueAsCorrectMap();
      TimeSeriesProperty<double> * log2 = dynamic_cast<TimeSeriesProperty<double> *>( WS2->mutableRun().getProperty("proton_charge") );
      std::map<DateAndTime, double> logMap2 = log2->valueAsCorrectMap();
      std::map<DateAndTime, double>::iterator it, it2;

      it = logMap.begin();
      it2 = logMap2.begin();
      for (; it != logMap.end(); )
      {
        //Same times within a millisecond
        //TS_ASSERT_DELTA( it->first, it2->first, DateAndTime::durationFromSeconds(1e-3));
        //Same times?
        TS_ASSERT_LESS_THAN( fabs(DateAndTime::secondsFromDuration(it->first - it2->first)), 1); //TODO: Fix the nexus file times here
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
        for (size_t i=0; i < events1.size(); i++)
        {
          TS_ASSERT_DELTA( events1[i].tof(), events2[i].tof(), 0.05);
          TS_ASSERT_DELTA( events1[i].pulseTime(), events2[i].pulseTime(), 1e9); //TODO:: Fix nexus start times
          //std::cout << (events1[i].pulseTime()-start)/1e9 << " - " << (events2[i].pulseTime()-start)/1e9 << " sec\n";
          //std::cout << "Pulse time diff " << (events1[i].pulseTime() - events2[i].pulseTime())/1e9 << " sec\n";
        }
      }

    }


  }

  void test_TOF_filtered_loading()
  {
    const std::string wsName = "test_filtering";
    const double filterStart = 45000;
    const double filterEnd = 59000;

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setProperty("FilterByTofMin", filterStart);
    ld.setProperty("FilterByTofMax", filterEnd);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT( ld.execute() );

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName); 

    auto eventList = outWs->getEventList(4348);
    auto events = eventList.getEvents();

    double max = events.begin()->tof();
    double min = events.begin()->tof();
    for(size_t j = 0; j < events.size(); ++j)
    {
      max = events[j].tof() > max ? events[j].tof() : max;
      min = events[j].tof() < min ? events[j].tof() : min;
    }
    TSM_ASSERT("The max TOF in the workspace should be equal to or less than the filtered cut-off", max <= filterEnd);
    TSM_ASSERT("The min TOF in the workspace should be equal to or greater than the filtered cut-off", min >= filterStart);
  }

  void test_partial_spectra_loading()
  {
    const std::string wsName = "test_partial_spectra_loading";
		std::vector<int32_t> specList;
		specList.push_back(13);
		specList.push_back(16);
		specList.push_back(21);
		specList.push_back(28);

    LoadEventNexus ld;
    ld.initialize();
    ld.setPropertyValue("OutputWorkspace", wsName);
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
		ld.setProperty("SpectrumList", specList);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver

    TS_ASSERT( ld.execute() );

    auto outWs = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsName); 

    TSM_ASSERT("The number of spectra in the workspace should be equal to the spectra filtered", outWs->getNumberHistograms()==specList.size());
    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(0)->getSpectrumNo()==13);
    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(1)->getSpectrumNo()==16);
    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(2)->getSpectrumNo()==21);
    TSM_ASSERT("Some spectra were not found in the workspace", outWs->getSpectrum(3)->getSpectrumNo()==28);

  }

	void test_Load_And_CompressEvents()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs_compressed";
    ld.initialize();
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace",outws_name);
    ld.setPropertyValue("Precount", "0");
    ld.setPropertyValue("CompressTolerance", "0.05");
    ld.setProperty<bool>("LoadMonitors", true); // For the next test, saving a load
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name) );
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS );
    //Pixels have to be padded
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 51200);
    //Events
    TS_ASSERT_EQUALS( WS->getNumberEvents(), 111274); // There are (slightly) fewer events
    for (size_t wi = 0; wi <  WS->getNumberHistograms(); wi++)
    {
      // Pixels with at least one event will have switched
      if (WS->getEventList(wi).getNumberEvents() > 0)
        TS_ASSERT_EQUALS( WS->getEventList(wi).getEventType(), WEIGHTED_NOTIME)
    }

  }

  void test_Monitors()
  {
    // Uses the workspace loaded in the last test to save a load execution
    std::string mon_outws_name = "cncs_compressed_monitors";
    auto & ads = AnalysisDataService::Instance();
    MatrixWorkspace_sptr WS = ads.retrieveWS<MatrixWorkspace>(mon_outws_name);
    //Valid WS and it is an MatrixWorkspace
    TS_ASSERT( WS );
    //Correct number of monitors found
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 3 );
    // Check some histogram data
    // TOF
    TS_ASSERT_EQUALS( (*WS->refX(0)).size(), 200002 );
    TS_ASSERT_DELTA( (*WS->refX(0))[1], 1.0, 1e-6 );
    // Data
    TS_ASSERT_EQUALS( WS->dataY(0).size(), 200001 );
    TS_ASSERT_DELTA( WS->dataY(0)[12], 0.0, 1e-6 );
    // Error
    TS_ASSERT_EQUALS( WS->dataE(0).size(), 200001 );
    TS_ASSERT_DELTA( WS->dataE(0)[12], 0.0, 1e-6 );
    // Check geometry for a monitor
    IDetector_const_sptr mon = WS->getDetector(2);
    TS_ASSERT( mon->isMonitor() );
    TS_ASSERT_EQUALS( mon->getID(), -3 );
    boost::shared_ptr<const IComponent> sample = WS->getInstrument()->getSample();
    TS_ASSERT_DELTA( mon->getDistance(*sample), 1.426, 1e-6 );

    // Check monitor workspace pointer held in main workspace
    TS_ASSERT_EQUALS( WS, ads.retrieveWS<MatrixWorkspace>("cncs_compressed")->monitorWorkspace() );
  }

  void doTestSingleBank(bool SingleBankPixelsOnly, bool Precount, std::string BankName = "bank36", bool willFail=false)
  {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "cncs";
    AnalysisDataService::Instance().remove(outws_name);
    ld.initialize();
    ld.setPropertyValue("Filename","CNCS_7860_event.nxs");
    ld.setPropertyValue("OutputWorkspace",outws_name);
    ld.setPropertyValue("BankName", BankName);
    ld.setProperty<bool>("SingleBankPixelsOnly", SingleBankPixelsOnly);
    ld.setProperty<bool>("Precount", Precount);
    ld.setProperty<bool>("LoadLogs", false); // Time-saver
    ld.execute();

    EventWorkspace_sptr WS;
    if (willFail)
    {
      TS_ASSERT( !ld.isExecuted() );
      return;
    }

    TS_ASSERT( ld.isExecuted() );
    TS_ASSERT_THROWS_NOTHING( WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name));
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS );
    if (!WS) return;
    //Pixels have to be padded
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), SingleBankPixelsOnly ? 1024 : 51200);
    //Events - there are fewer now.
    TS_ASSERT_EQUALS( WS->getNumberEvents(), 7274);
  }

  void test_SingleBank_AllPixels()
  {
    doTestSingleBank(false, false);
  }


  void test_SingleBank_AllPixels_Precount()
  {
    doTestSingleBank(false, true);
  }

  void test_SingleBank_PixelsOnlyInThatBank_Precount()
  {
    doTestSingleBank(true, true);
  }

  void test_SingleBank_ThatDoesntExist()
  {
    doTestSingleBank(false, false, "bankDoesNotExist", true);
  }

  void test_SingleBank_with_no_events()
  {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING( load.initialize() );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("Filename", "HYSA_12509.nxs.h5") );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("BankName", "bank10") );
    const std::string outws("AnEmptyWS");
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("OutputWorkspace", outws) );
    if ( !load.execute() )
    {
      TS_FAIL("LoadEventNexus shouldn't fail to load an empty bank");
      return;
    }

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    TS_ASSERT_EQUALS( ws->getNumberEvents(), 0 );
  }

  void test_instrument_inside_nexus_file()
  {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING( load.initialize() );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("Filename", "HYSA_12509.nxs.h5") );
    const std::string outws("InstInNexus");
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("OutputWorkspace", outws) );
    TS_ASSERT( load.execute() );

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    auto inst = ws->getInstrument();
    TS_ASSERT( inst->getFilename().empty() ); // This is how we know we got it from inside the nexus file
    TS_ASSERT_EQUALS( inst->getName(), "HYSPECA" );
    TS_ASSERT_EQUALS( inst->getValidFromDate(), std::string("2011-Jul-20 17:02:48.437294000") );
    TS_ASSERT_EQUALS( inst->getNumberDetectors(), 20483 );
    TS_ASSERT_EQUALS( inst->baseInstrument()->numMonitors(), 3 );
    auto params = inst->getParameterMap();
    TS_ASSERT_EQUALS( params->size(), 49);
    TS_ASSERT_EQUALS( params->getString(inst.get(), "deltaE-mode"), "direct");
  }

  void test_instrument_and_default_param_loaded_when_inst_not_in_nexus_file()
  {
    LoadEventNexus load;
    TS_ASSERT_THROWS_NOTHING( load.initialize() );
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("Filename", "CNCS_7860_event.nxs") );
    load.setProperty<bool>("LoadLogs", false); // Time-saver
    const std::string outws("InstNotInNexus");
    TS_ASSERT_THROWS_NOTHING( load.setPropertyValue("OutputWorkspace", outws) );
    TS_ASSERT( load.execute() );

    auto ws = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws);
    auto inst = ws->getInstrument();
    TS_ASSERT( !inst->getFilename().empty()); // This is how we know we didn't get it from inside the nexus file
    TS_ASSERT_EQUALS( inst->getName(), "CNCS" );
    TS_ASSERT_EQUALS( inst->getNumberDetectors(), 51203 );
    TS_ASSERT_EQUALS( inst->baseInstrument()->numMonitors(), 3 );

    // check that CNCS_Parameters.xml has been loaded
    auto params = inst->getParameterMap();
    TS_ASSERT_EQUALS( params->getString(inst.get(), "deltaE-mode"), "direct");
  }

  /** Test with a particular ARCS file that has 2 preprocessors,
   * meaning different-sized pulse ID files.
   * DISABLED AS THE FILE ISN'T IN THE REPOSITORY
   */
  void xtest_MultiplePreprocessors()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string outws_name = "arcs";
    ld.initialize();
    try
    {
      ld.setPropertyValue("Filename","ARCS_12954_event.nxs");
    }
    catch (...)
    {
      std::cout << "Skipping test since file does not exist.";
      return;
    }
    ld.setPropertyValue("OutputWorkspace",outws_name);
    ld.setPropertyValue("CompressTolerance", "-1");
    ld.execute();
    TS_ASSERT( ld.isExecuted() );

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outws_name) );
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS );
    if (!WS) return;
    TS_ASSERT_EQUALS( WS->getNumberHistograms(), 117760);
    TS_ASSERT_EQUALS( WS->getNumberEvents(), 10730347);
    for (size_t wi = 0; wi <  WS->getNumberHistograms(); wi++)
    {
      // Times are NON-zero for ALL pixels.
      if (WS->getEventList(wi).getNumberEvents() > 0)
      {
        int64_t nanosec = WS->getEventList(wi).getEvents()[0].pulseTime().totalNanoseconds();
        TS_ASSERT_DIFFERS( nanosec, 0)
        if (nanosec==0) { std::cout << "Failure at WI " << wi << std::endl; return; }
      }
    }
  }

  void test_start_and_end_time_filtered_loading_meta_data_only()
  {
    const bool metadataonly = true;
    do_test_filtering_start_and_end_filtered_loading(metadataonly);
  }

  void test_start_and_end_time_filtered_loading()
  {
    const bool metadataonly = false;
    do_test_filtering_start_and_end_filtered_loading(metadataonly);
  }

  void testSimulatedFile()
  {
    Mantid::API::FrameworkManager::Instance();
    LoadEventNexus ld;
    std::string wsname = "ARCS_sim";
    ld.initialize();
    ld.setPropertyValue("Filename", "ARCS_sim_event.nxs");
    ld.setPropertyValue("OutputWorkspace", wsname);
    ld.setProperty("BankName","bank27");
    ld.setProperty("SingleBankPixelsOnly",false);
    ld.setProperty("LoadLogs",false);
    TS_ASSERT( ld.execute() );

    EventWorkspace_sptr WS;
    TS_ASSERT_THROWS_NOTHING(
        WS = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsname) );
    //Valid WS and it is an EventWorkspace
    TS_ASSERT( WS );
    if (!WS) return;
    const auto numHist = WS->getNumberHistograms();
    TS_ASSERT_EQUALS( numHist, 117760);
    TS_ASSERT_EQUALS( WS->getNumberEvents(), 2);
    for (size_t wi = 0; wi <  numHist; wi+=5000)
    {
      // All events should be weighted events for simulated data
      TS_ASSERT_EQUALS( WS->getEventList(wi).getEventType(), WEIGHTED);
    }
    // Check one event
    TS_ASSERT_DELTA(WS->getEventList(26798).getWeightedEvents()[0].weight(),
        1.8124e-11, 1.0e-4);
    TS_ASSERT_EQUALS(WS->getEventList(26798).getWeightedEvents()[0].tof(), 1476.0);
  }

};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class LoadEventNexusTestPerformance : public CxxTest::TestSuite
{
public:
  void testDefaultLoad()
  {
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    loader.setPropertyValue("OutputWorkspace", "ws");
    TS_ASSERT( loader.execute() );
  }
};

#endif /*LOADEVENTNEXUSTEST_H_*/



