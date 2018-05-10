/*
 * FilterByTimeTest.h
 *
 *  Created on: Sep 14, 2010
 *      Author: janik
 */

#ifndef FILTERBYTIMETEST_H_
#define FILTERBYTIMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/FilterByTime.h"
#include "MantidDataHandling/LoadEventNexus.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

class FilterByTimeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterByTimeTest *createSuite() { return new FilterByTimeTest(); }
  static void destroySuite(FilterByTimeTest *suite) { delete suite; }

  FilterByTimeTest() {
    inWS = "filterbytime_input";
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspace(4, 1);
    // Add proton charge
    TimeSeriesProperty<double> *pc =
        new TimeSeriesProperty<double>("proton_charge");
    pc->setUnits("picoCoulomb");
    DateAndTime run_start("2010-01-01T00:00:00"); // NOTE This run_start is
                                                  // hard-coded in
                                                  // WorkspaceCreationHelper.
    for (double i = 0; i < 100; i++)
      pc->addValue(run_start + i, 1.0);
    ws->mutableRun().addProperty(pc);
    ws->mutableRun().integrateProtonCharge();

    AnalysisDataService::Instance().add(inWS, ws);
  }

  ~FilterByTimeTest() override { AnalysisDataService::Instance().clear(); }

  void testTooManyParams() {
    EventWorkspace_sptr ws =
        WorkspaceCreationHelper::createEventWorkspace(1, 1);
    AnalysisDataService::Instance().addOrReplace("eventWS", ws);

    // Do the filtering now.
    FilterByTime alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", "eventWS");
    alg.setPropertyValue("OutputWorkspace", "out");
    alg.setPropertyValue("StopTime", "120");
    alg.setPropertyValue("AbsoluteStartTime", "2010");
    alg.execute();
    TS_ASSERT(!alg.isExecuted());

    FilterByTime alg2;
    alg2.initialize();
    alg2.setPropertyValue("InputWorkspace", "eventWS");
    alg2.setPropertyValue("OutputWorkspace", "out");
    alg2.setPropertyValue("StartTime", "60");
    alg2.setPropertyValue("StopTime", "120");
    alg2.setPropertyValue("AbsoluteStartTime", "2010");
    alg2.execute();
    TS_ASSERT(!alg2.isExecuted());

    FilterByTime alg3;
    alg3.initialize();
    alg3.setPropertyValue("InputWorkspace", "eventWS");
    alg3.setPropertyValue("OutputWorkspace", "out");
    alg3.setPropertyValue("StopTime", "120");
    alg3.setPropertyValue("AbsoluteStartTime", "2010");
    alg3.setPropertyValue("AbsoluteStopTime", "2010-03");
    alg3.execute();
    TS_ASSERT(!alg3.isExecuted());
  }

  void test_relative_time() {
    FilterByTime alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    const std::string outWS("relative");
    alg.setProperty("OutputWorkspace", outWS);
    alg.setProperty("StartTime", 40.5);
    alg.setProperty("StopTime", 75.0);
    TS_ASSERT(alg.execute());

    EventWorkspace_sptr input =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWS);
    EventWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWS);
    // Things that haven't changed
    TS_ASSERT_EQUALS(output->blocksize(), input->blocksize());
    TS_ASSERT_EQUALS(output->getNumberHistograms(),
                     input->getNumberHistograms());
    // Things that changed
    TS_ASSERT_LESS_THAN(output->getNumberEvents(), input->getNumberEvents());
    TS_ASSERT_EQUALS(output->getNumberEvents(), 136);
    // Proton charge is lower
    TS_ASSERT_LESS_THAN(output->run().getProtonCharge(),
                        input->run().getProtonCharge());
    // Event distribution is uniform so the following should hold true
    TS_ASSERT_DELTA(output->run().getProtonCharge() /
                        input->run().getProtonCharge(),
                    136.0 / 400.0, 0.01);

    // Test 'null' filter
    FilterByTime alg2;
    alg2.initialize();
    alg2.setProperty("InputWorkspace", inWS);
    alg2.setProperty("OutputWorkspace", outWS);
    alg2.setProperty("StartTime", 0.0);
    alg2.setProperty("StopTime", 101.0);
    TS_ASSERT(alg2.execute());
    output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWS);
    TS_ASSERT_EQUALS(output->getNumberEvents(), input->getNumberEvents());
    TS_ASSERT_EQUALS(output->run().getProtonCharge(),
                     input->run().getProtonCharge());
  }

  void test_absolute_time() {
    FilterByTime alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    const std::string outWS("absolute");
    alg.setProperty("OutputWorkspace", outWS);
    alg.setPropertyValue("AbsoluteStartTime", "2010-01-01T00:00:50");
    alg.setPropertyValue("AbsoluteStopTime", "2010-01-01T00:01:10");
    TS_ASSERT(alg.execute());

    EventWorkspace_sptr input =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWS);
    EventWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWS);
    // Things that haven't changed
    TS_ASSERT_EQUALS(output->blocksize(), input->blocksize());
    TS_ASSERT_EQUALS(output->getNumberHistograms(),
                     input->getNumberHistograms());
    // Things that changed
    TS_ASSERT_LESS_THAN(output->getNumberEvents(), input->getNumberEvents());
    TS_ASSERT_EQUALS(output->getNumberEvents(), 80);
    // Proton charge is lower
    TS_ASSERT_LESS_THAN(output->run().getProtonCharge(),
                        input->run().getProtonCharge());
    // Event distribution is uniform so the following should hold true
    TS_ASSERT_DELTA(output->run().getProtonCharge() /
                        input->run().getProtonCharge(),
                    80.0 / 400.0, 0.01);

    // Test 'null' filter
    FilterByTime alg2;
    alg2.initialize();
    alg2.setProperty("InputWorkspace", inWS);
    alg2.setProperty("OutputWorkspace", outWS);
    alg2.setPropertyValue("AbsoluteStartTime", "2009-12-31T00:00:00");
    alg2.setPropertyValue("AbsoluteStopTime", "2010-01-02T00:01:10");
    TS_ASSERT(alg2.execute());
    output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outWS);
    TS_ASSERT_EQUALS(output->getNumberEvents(), input->getNumberEvents());
    TS_ASSERT_EQUALS(output->run().getProtonCharge(),
                     input->run().getProtonCharge());
  }

  void test_same_output_and_input_workspaces() {
    FilterByTime alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", inWS);
    alg.setProperty("OutputWorkspace", inWS);
    alg.setProperty("StartTime", 20.5);
    alg.setProperty("StopTime", 70.5);
    TS_ASSERT(alg.execute());

    EventWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(inWS);
    TS_ASSERT_LESS_THAN(0, outWS->getNumberEvents());
  }

private:
  std::string inWS;
};

//------------------------------------------------------------------------------
// Performance test
//------------------------------------------------------------------------------

class FilterByTimeTestPerformance : public CxxTest::TestSuite {
public:
  static FilterByTimeTestPerformance *createSuite() {
    return new FilterByTimeTestPerformance();
  }
  static void destroySuite(FilterByTimeTestPerformance *suite) { delete suite; }

  FilterByTimeTestPerformance() {
    LoadEventNexus loader;
    loader.initialize();
    loader.setPropertyValue("Filename", "CNCS_7860_event.nxs");
    const std::string outWS("FilterByTimeTestPerformance");
    loader.setPropertyValue("OutputWorkspace", outWS);
    loader.execute();

    alg.initialize();
    alg.setProperty("InputWorkspace", outWS);
    alg.setProperty("OutputWorkspace", "anon");
    alg.setProperty("StartTime", 60.0);
    alg.setProperty("StopTime", 120.0);
  }

  ~FilterByTimeTestPerformance() override {
    AnalysisDataService::Instance().clear();
  }

  void test_filtering() { alg.execute(); }

private:
  FilterByTime alg;
};

#endif /* FILTERBYTIMETEST_H_ */
