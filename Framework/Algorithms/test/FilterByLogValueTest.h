// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FILTERBYLOGVALUETEST_H_
#define FILTERBYLOGVALUETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/FilterByLogValue.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using Mantid::Types::Core::DateAndTime;

class FilterByLogValueTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FilterByLogValueTest *createSuite() {
    return new FilterByLogValueTest();
  }
  static void destroySuite(FilterByLogValueTest *suite) { delete suite; }

  FilterByLogValueTest() {}

  void test_validators() {
    FilterByLogValue alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    // InputWorkspace has to be an EventWorkspace
    TS_ASSERT_THROWS(
        alg.setProperty("InputWorkspace",
                        WorkspaceCreationHelper::create2DWorkspace(1, 1)),
        const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspace", WorkspaceCreationHelper::createEventWorkspace()));

    // LogName must not be empty
    TS_ASSERT_THROWS(alg.setProperty("LogName", ""),
                     const std::invalid_argument &);

    // TimeTolerance cannot be negative
    TS_ASSERT_THROWS(alg.setProperty("TimeTolerance", -0.1),
                     const std::invalid_argument &);
    // ... but it can be zero
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("TimeTolerance", 0.0));

    // LogBoundary must be one of "Centre" and "Left"
    TS_ASSERT_THROWS(alg.setProperty("LogBoundary", ""),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(alg.setProperty("LogBoundary", "Middle"),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary", "Left"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogBoundary", "Centre"));
  }

  void test_validateInputs() {
    // Create and event workspace. We don't care what data is in it.
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspace();
    // Add a single-number log
    ws->mutableRun().addProperty("SingleValue", 5);
    // Add a time-series property
    auto tsp = new TimeSeriesProperty<double>("TSP");
    tsp->addValue(DateAndTime::getCurrentTime(), 9.9);
    ws->mutableRun().addLogData(tsp);

    FilterByLogValue alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", ws));

    // Check protest when non-existent log is set
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "NotThere"));
    auto errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS(errorMap.size(), 1);
    TS_ASSERT_EQUALS(errorMap.begin()->first, "LogName");

    // Check protest when single-value log is set
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "SingleValue"));
    errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS(errorMap.size(), 1);
    TS_ASSERT_EQUALS(errorMap.begin()->first, "LogName");

    // Check protest when tsp log given, but min value greate than max
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "TSP"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MinimumValue", 2.0));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumValue", 1.0));
    errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS(errorMap.size(), 2);
    TS_ASSERT_EQUALS(errorMap.begin()->first, "MaximumValue");
    TS_ASSERT_EQUALS(errorMap.rbegin()->first, "MinimumValue");

    // Check it's happy when that's been remedied
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("MaximumValue", 3.0));
    errorMap = alg.validateInputs();
    TS_ASSERT(errorMap.empty());
  }

  /** Create a workspace with:
   * events at times 0,1,2,...99
   * LOGS:
   *  temp = 10 C at 10 sec up to 50C at 50 sec, every 10 seconds
   *  press = -10 seconds to +150 seconds, every 10 seconds
   *
   * @param add_proton_charge
   */
  EventWorkspace_sptr createInputWS(bool add_proton_charge = true) {
    // Default Event Workspace with times from 0-99
    EventWorkspace_sptr ew = WorkspaceCreationHelper::createEventWorkspace2();

    DateAndTime run_start("2010-01-01T00:00:00"); // NOTE This run_start is
                                                  // hard-coded in
                                                  // WorkspaceCreationHelper.

    TimeSeriesProperty<double> *temp;
    temp = new TimeSeriesProperty<double>("temp");
    // 10 C at 10 sec up to 50C at 50 sec
    for (double i = 10; i <= 50; i += 10)
      temp->addValue(run_start + i, i);
    ew->mutableRun().addProperty(temp);

    // Log that goes before and after the pulse times
    TimeSeriesProperty<double> *press;
    press = new TimeSeriesProperty<double>("press");
    for (double i = -10; i <= 150; i += 10)
      press->addValue(run_start + i, i);
    ew->mutableRun().addProperty(press);

    if (add_proton_charge) {
      TimeSeriesProperty<double> *pc =
          new TimeSeriesProperty<double>("proton_charge");
      pc->setUnits("picoCoulomb");
      for (double i = 0; i < 100; i++)
        pc->addValue(run_start + i, 1.0);
      ew->mutableRun().addProperty(pc);
    }

    TimeSeriesProperty<double> *single;

    // Single-entry logs with points at different places
    single = new TimeSeriesProperty<double>("single_middle");
    single->addValue(run_start + 30.0, 1.0);
    ew->mutableRun().addProperty(single);

    single = new TimeSeriesProperty<double>("single_before");
    single->addValue(run_start - 15.0, 1.0);
    ew->mutableRun().addProperty(single);

    single = new TimeSeriesProperty<double>("single_after");
    single->addValue(run_start + 200.0, 1.0);
    ew->mutableRun().addProperty(single);

    // Finalize the needed stuff
    WorkspaceCreationHelper::eventWorkspace_Finalize(ew);

    return ew;
  }

  /** Run the algorithm on a workspace generated by createInputWS()
   *
   * @param log_name
   * @param min
   * @param max
   * @param seconds_kept
   * @param add_proton_charge
   * @param do_in_place
   * @param PulseFilter :: PulseFilter parameter
   */
  void do_test_fake(std::string log_name, double min, double max,
                    int seconds_kept, bool add_proton_charge = true,
                    bool do_in_place = false, bool PulseFilter = false) {
    EventWorkspace_sptr ew = createInputWS(add_proton_charge);
    std::string inputName = "input_filtering";
    AnalysisDataService::Instance().addOrReplace(
        inputName, boost::dynamic_pointer_cast<MatrixWorkspace>(ew));

    // Save some of the starting values
    size_t start_blocksize = ew->blocksize();
    size_t num_events = ew->getNumberEvents();
    const double CURRENT_CONVERSION = 1.e-6 / 3600.;
    double start_proton_charge =
        ew->run().getProtonCharge() / CURRENT_CONVERSION;
    size_t num_sample_logs = ew->run().getProperties().size();
    TS_ASSERT_EQUALS(num_events, 100 * 2 * ew->getNumberHistograms());
    if (add_proton_charge) {
      TS_ASSERT_EQUALS(start_proton_charge, 100.0);
    }

    // Do the filtering now.
    FilterByLogValue alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspace", inputName);

    std::string outputWS = "output_filtering";
    if (do_in_place)
      outputWS = inputName;

    alg.setPropertyValue("OutputWorkspace", outputWS);
    alg.setPropertyValue("LogName", log_name);
    // We set the minimum high enough to cut out some real charge too, not just
    // zeros.
    alg.setProperty("MinimumValue", min);
    alg.setProperty("MaximumValue", max);
    alg.setPropertyValue("TimeTolerance", "3e-3");
    alg.setProperty("PulseFilter", PulseFilter);
    alg.execute();
    TS_ASSERT(alg.isExecuted());

    // Retrieve Workspace changed
    EventWorkspace_sptr outWS;
    outWS =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>(outputWS);
    TS_ASSERT(outWS); // workspace is loaded
    if (!outWS)
      return;

    // The events match the expected number
    TS_ASSERT_EQUALS(outWS->getNumberEvents(),
                     seconds_kept * 2 * outWS->getNumberHistograms());

    // Things that haven't changed
    TS_ASSERT_EQUALS(outWS->blocksize(), start_blocksize);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 50);
    TS_ASSERT_EQUALS(outWS->run().getProperties().size(), num_sample_logs);

    // Proton charge is lower
    if (add_proton_charge) {
      TS_ASSERT_EQUALS(outWS->run().getProtonCharge() / CURRENT_CONVERSION,
                       seconds_kept * 1.0);
    }

    AnalysisDataService::Instance().remove(outputWS);
  }

  void test_filter_InPlace() {
    // Keep a 11 second block (20 to 30 inclusively)
    // But do it in place on the event workspace
    do_test_fake("temp", 19.5, 30.5, 11, true, true);
    do_test_fake("press", 19.5, 30.5, 11, true, true);
  }

  /*** The next tests will be done off-place **/

  void test_filter_keep_part_of_a_log() {
    // Keep a 11 second block (20 to 30 inclusively)
    do_test_fake("temp", 19.5, 30.5, 11, true, false);
    do_test_fake("press", 19.5, 30.5, 11, true, false);
  }

  void test_filter_beginning_value_is_implied() {
    // Log starts at 10 C at second=10; We assume temp constant at 10 before
    // that time.
    // 0-30 secs inclusive = 31 seconds
    do_test_fake("temp", 5, 30.5, 31);
    // But this one was 0 at 0 seconds, so no implied constancy is used
    // Therefore, 10-30 seconds inclusive
    do_test_fake("press", 5, 30.5, 21);
  }

  void test_filter_beginning_value_but_no_proton_charge() {
    // Same as previous test but there is no proton_charge to give the start and
    // end times.
    // This time, it starts at the first point (10) and ends at (30) giving 21
    // points
    do_test_fake("temp", 5, 30.5, 21, false);
  }

  void test_filter_ending_value_is_implied() {
    // Log starts at 10 C at second=10; We assume temp constant at 10 before
    // that time.
    // 30-99 secs inclusive = 70 secs
    do_test_fake("temp", 29.5, 150, 70);
  }

  /** Single values are to be considered constant through all time
   * Therefore, all these tests should keep all events:
   */
  void test_filter_single_value_in_the_middle() {
    do_test_fake("single_middle", 0, 2.0, 100);
  }

  void test_filter_single_value_before() {
    do_test_fake("single_before", 0, 2.0, 100);
  }

  void test_filter_single_value_after() {
    do_test_fake("single_after", 0, 2.0, 100);
  }

  /** This test will not keep any events because you are outside the specified
   * range.
   */
  void test_filter_single_value_outside_range1() {
    do_test_fake("single_middle", 2.0, 4.0, 0);
    do_test_fake("single_before", 2.0, 4.0, 0);
    do_test_fake("single_after", 2.0, 4.0, 0);
  }

  void test_pulseFilter() {
    // We filter out exactly the times of the temp log.
    // It has 5 entries, leaving 95 seconds of events
    do_test_fake("temp", 0, 0, 95, true, true /* in place*/,
                 true /*PulseFilter*/);
    do_test_fake("temp", 0, 0, 95, true, false /* not in place*/,
                 true /*PulseFilter*/);
    // Filter on an entry with only one point
    do_test_fake("single_middle", 0, 0, 99, true, false /* not in place*/,
                 true /*PulseFilter*/);
  }

  std::size_t min_max_helper(bool useMin, bool useMax, double min, double max) {
    FilterByLogValue alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", createInputWS());
    alg.setProperty("OutputWorkspace", "dontmatter");
    alg.setProperty("LogName", "press");
    alg.setProperty("LogBoundary", "Left");
    if (useMin)
      alg.setProperty("MinimumValue", min);
    if (useMax)
      alg.setProperty("MaximumValue", max);

    TS_ASSERT(alg.execute());

    EventWorkspace_const_sptr outWS = alg.getProperty("OutputWorkspace");
    return outWS->getNumberEvents();
  }

  // Test that leaving one or both of MinimumValue & MaximumValue properties
  // empty does the right thing
  void test_default_min_max() {
    // Test that leaving both empty gives back an unchanged workspace
    TS_ASSERT_EQUALS(min_max_helper(false, false, 0.0, 0.0), 10000);
    // Test that setting min higher that max value in log wipes out all events
    TS_ASSERT_EQUALS(min_max_helper(true, false, 200.0, 0.0), 0);
    // Test that setting max lower that min value in log wipes out all events
    TS_ASSERT_EQUALS(min_max_helper(false, true, 0.0, -20.0), 0);
    // Test that default max on it's own works for an in-range min
    TS_ASSERT_EQUALS(min_max_helper(true, false, 70.0, 0.0), 3000);
    // Test that default min on it's own works for an in-range max
    TS_ASSERT_EQUALS(min_max_helper(false, true, 0.0, 70.0), 8000);
  }

private:
  EventWorkspace_sptr WS;
};

#endif
