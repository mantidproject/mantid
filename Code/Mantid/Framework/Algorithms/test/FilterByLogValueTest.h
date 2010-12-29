/*
 * FilterByLogValueTest.h
 *
 *  Created on: Sep 15, 2010
 *      Author: janik
 */

#ifndef FILTERBYLOGVALUETEST_H_
#define FILTERBYLOGVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FilterByLogValue.h"
#include "MantidDataHandling/LoadEventPreNeXus.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidAPI/AlgorithmManager.h"
#include "WorkspaceCreationHelper.hh"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;

class FilterByLogValueTest : public CxxTest::TestSuite
{
public:
  FilterByLogValueTest()
  {
    inputWS = "eventWS";
  }


  /** Setup for loading raw data */
  void setUp_Event()
  {
    IAlgorithm_sptr loader = AlgorithmManager::Instance().create("LoadSNSEventNexus");
    loader->initialize();
    loader->setPropertyValue("Filename", "CNCS_7860_event.nxs");
    loader->setPropertyValue("OutputWorkspace", inputWS);
    loader->execute();
    TS_ASSERT (loader->isExecuted() );
  }



  void doTest(std::string outputWS)
  {
    //Retrieve Workspace
    this->setUp_Event();
    WS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(inputWS));
    TS_ASSERT( WS ); //workspace is loaded

    size_t start_blocksize = WS->blocksize();
    size_t num_events = WS->getNumberEvents();
    double start_proton_charge = WS->run().getProtonCharge();
    size_t num_sample_logs = WS->run().getProperties().size();
    TS_ASSERT_EQUALS( num_events, 112266 );
    //Do the filtering now.
    FilterByLogValue * alg = new FilterByLogValue();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputWS);
    alg->setPropertyValue("OutputWorkspace", outputWS);
    alg->setPropertyValue("LogName", "proton_charge");
    //We set the minimum high enough to cut out some real charge too, not just zeros.
    alg->setPropertyValue("MinimumValue", "1.e7");
    alg->setPropertyValue("MaximumValue", "1e20");
    alg->setPropertyValue("TimeTolerance", "4e-12");

    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT( outWS ); //workspace is loaded

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), WS->blocksize());
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), WS->getNumberHistograms());

    //There should be some events
    TS_ASSERT_LESS_THAN( 0, outWS->getNumberEvents());

    TS_ASSERT_LESS_THAN( outWS->getNumberEvents(), num_events);
    TS_ASSERT_DELTA(  outWS->getNumberEvents() , 83434, 100);

    //Proton charge is lower
    TS_ASSERT_EQUALS( outWS->run().getProperties().size(), num_sample_logs);
    TS_ASSERT_LESS_THAN( outWS->run().getProtonCharge(), start_proton_charge );
    // But not 0
    TS_ASSERT_LESS_THAN( 0, outWS->run().getProtonCharge());

    //Still has a spectraDetectorMap;
    outWS->spectraMap();

  }

  void test_exec_renamed()
  {
    doTest(inputWS + "_filtered");
    AnalysisDataService::Instance().remove(inputWS);
    AnalysisDataService::Instance().remove(inputWS + "_filtered");
  }

  void test_exec_inplace()
  {
    doTest(inputWS);
    AnalysisDataService::Instance().remove(inputWS);
  }



  void do_test_fake(std::string log_name, double min, double max, int seconds_kept,
      bool add_proton_charge = true, bool do_in_place = false)
  {
    // Default Event Workspace with times from 0-99
    EventWorkspace_sptr ew = WorkspaceCreationHelper::CreateEventWorkspace2();

    TimeSeriesProperty<double> * temp;
    temp = new TimeSeriesProperty<double>("temp");
    //10 C at 10 sec up to 50C at 50 sec
    for (int i=10; i<=50; i +=10)
      temp->addValue( DateAndTime(i, 0), i);
    ew->mutableRun().addProperty( temp );

    // Log that goes before and after the pulse times
    TimeSeriesProperty<double> * press;
    press = new TimeSeriesProperty<double>("press");
    for (int i=-10; i<=150; i +=10)
      press->addValue( DateAndTime(i, 0), i);
    ew->mutableRun().addProperty( press );


    if (add_proton_charge)
    {
      TimeSeriesProperty<double> * pc = new TimeSeriesProperty<double>("proton_charge");
      for (int i=0; i<100; i++)
        pc->addValue( DateAndTime(i, 0), 1.0);
      ew->mutableRun().addProperty( pc );
    }

    TimeSeriesProperty<double> * single;

    // Single-entry logs with points at different places
    single = new TimeSeriesProperty<double>("single_middle");
    single->addValue( DateAndTime(30, 0), 1.0);
    ew->mutableRun().addProperty( single );

    single = new TimeSeriesProperty<double>("single_before");
    single->addValue( DateAndTime(-15, 0), 1.0);
    ew->mutableRun().addProperty( single );

    single = new TimeSeriesProperty<double>("single_after");
    single->addValue( DateAndTime(200, 0), 1.0);
    ew->mutableRun().addProperty( single );

    // Finalize the needed stuff
    WorkspaceCreationHelper::EventWorkspace_Finalize(ew);

    std::string inputName = "input_filtering";
    AnalysisDataService::Instance().addOrReplace(inputName, boost::dynamic_pointer_cast<MatrixWorkspace>(ew) );


    // Save some of the starting values
    size_t start_blocksize = ew->blocksize();
    size_t num_events = ew->getNumberEvents();
    const double CURRENT_CONVERSION = 1.e-6 / 3600.;
    double start_proton_charge = ew->run().getProtonCharge()/CURRENT_CONVERSION;
    size_t num_sample_logs = ew->run().getProperties().size();
    TS_ASSERT_EQUALS( num_events, 100 * 2 * ew->getNumberHistograms());
    if (add_proton_charge)
    {
      TS_ASSERT_EQUALS( start_proton_charge, 100.0);
    }



    //Do the filtering now.
    FilterByLogValue * alg = new FilterByLogValue();
    alg->initialize();
    alg->setPropertyValue("InputWorkspace", inputName);

    std::string outputWS = "output_filtering";
    if (do_in_place) outputWS = inputName;

    alg->setPropertyValue("OutputWorkspace", outputWS);
    alg->setPropertyValue("LogName", log_name);
    //We set the minimum high enough to cut out some real charge too, not just zeros.
    alg->setProperty("MinimumValue", min);
    alg->setProperty("MaximumValue", max);
    alg->setPropertyValue("TimeTolerance", "3e-3");

    alg->execute();
    TS_ASSERT( alg->isExecuted() );

    //Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS = boost::dynamic_pointer_cast<EventWorkspace>(AnalysisDataService::Instance().retrieve(outputWS));
    TS_ASSERT( outWS ); //workspace is loaded
    if (!outWS) return;

    // The events match the expected number
    TS_ASSERT_EQUALS( outWS->getNumberEvents(), seconds_kept * 2 * outWS->getNumberHistograms() );

    //Things that haven't changed
    TS_ASSERT_EQUALS( outWS->blocksize(), start_blocksize);
    TS_ASSERT_EQUALS( outWS->getNumberHistograms(), 50);
    TS_ASSERT_EQUALS( outWS->run().getProperties().size(), num_sample_logs);

    //Proton charge is lower
    if (add_proton_charge)
    {
      TS_ASSERT_EQUALS( outWS->run().getProtonCharge()/CURRENT_CONVERSION, seconds_kept * 1.0 );
    }

    //Still has a spectraDetectorMap;
    outWS->spectraMap();

    AnalysisDataService::Instance().remove(outputWS);
  }


  void test_filter_InPlace()
  {
    // Keep a 11 second block (20 to 30 inclusively)
    // But do it in place on the event workspace
    do_test_fake("temp", 19.5, 30.5, 11, true, true);
    do_test_fake("press", 19.5, 30.5, 11, true, true);
  }

  /*** The next tests will be done off-place **/


  void test_filter_keep_part_of_a_log()
  {
    // Keep a 11 second block (20 to 30 inclusively)
    do_test_fake("temp", 19.5, 30.5, 11, true, false);
    do_test_fake("press", 19.5, 30.5, 11, true, false);
  }

  void test_filter_beginning_value_is_implied()
  {
    // Log starts at 10 C at second=10; We assume temp constant at 10 before that time.
    // 0-30 secs inclusive = 31 seconds
    do_test_fake("temp", 5, 30.5, 31);
    // But this one was 0 at 0 seconds, so no implied constancy is used
    // Therefore, 10-30 seconds inclusive
    do_test_fake("press", 5, 30.5, 21);
  }

  void test_filter_beginning_value_but_no_proton_charge()
  {
    // Same as previous test but there is no proton_charge to give the start and end times.
    // This time, it starts at the first point (10) and ends at (30) giving 21 points
    do_test_fake("temp", 5, 30.5, 21, false);
  }


  void test_filter_ending_value_is_implied()
  {
    // Log starts at 10 C at second=10; We assume temp constant at 10 before that time.
    // 30-99 secs inclusive = 70 secs
    do_test_fake("temp", 29.5, 150, 70);
  }

  /** Single values are to be considered constant through all time
   * Therefore, all these tests should keep all events:
   */
  void test_filter_single_value_in_the_middle()
  {
    do_test_fake("single_middle", 0, 2.0, 100);
  }

  void test_filter_single_value_before()
  {
    do_test_fake("single_before", 0, 2.0, 100);
  }

  void test_filter_single_value_after()
  {
    do_test_fake("single_after", 0, 2.0, 100);
  }

  /** This test will not keep any events because you are outside the specified range.
   */
  void test_filter_single_value_outside_range1()
  {
    do_test_fake("single_middle", 2.0, 4.0, 0);
    do_test_fake("single_before", 2.0, 4.0, 0);
    do_test_fake("single_after", 2.0, 4.0, 0);
  }



private:
  std::string inputWS;
  EventWorkspace_sptr WS;


};


#endif


