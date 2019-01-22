// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MERGERUNSTEST_H_
#define MANTID_ALGORITHMS_MERGERUNSTEST_H_

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/SpectrumInfo.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidAlgorithms/MergeRuns.h"
#include "MantidAlgorithms/Rebin.h"
#include "MantidAlgorithms/RunCombinationHelpers/RunCombinationHelper.h"
#include "MantidAlgorithms/RunCombinationHelpers/SampleLogsBehaviour.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidGeometry/Instrument.h"
#include "MantidGeometry/Instrument/DetectorInfo.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTypes/SpectrumDefinition.h"
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <stdarg.h>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;
class MergeRunsTest : public CxxTest::TestSuite {

private:
  MergeRuns merge;

  /// Helper method to add an 'nperiods' log value to each workspace in a group.
  void add_periods_logs(WorkspaceGroup_sptr ws, bool calculateNPeriods = true,
                        int nperiods = -1) {
    if (calculateNPeriods) {
      nperiods = static_cast<int>(ws->size());
    }
    for (size_t i = 0; i < ws->size(); ++i) {
      MatrixWorkspace_sptr currentWS =
          boost::dynamic_pointer_cast<MatrixWorkspace>(ws->getItem(i));

      PropertyWithValue<int> *nperiodsProp =
          new PropertyWithValue<int>("nperiods", nperiods);
      currentWS->mutableRun().addLogData(nperiodsProp);
      PropertyWithValue<int> *currentPeriodsProp =
          new PropertyWithValue<int>("current_period", static_cast<int>(i + 1));
      currentWS->mutableRun().addLogData(currentPeriodsProp);
    }
  }

  /// Helper to fabricate a workspace group consisting of equal sized
  /// matrixworkspaces. BUT WITHOUT MULTIPERIOD LOGS.
  WorkspaceGroup_sptr create_good_workspace_group() {
    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    // a->setName("a1");
    // b->setName("b1");
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    // group->setName("group1");
    group->addWorkspace(a);
    group->addWorkspace(b); // No multiperiod logs added.
    AnalysisDataService::Instance().addOrReplace("a1", a);
    AnalysisDataService::Instance().addOrReplace("b1", b);
    AnalysisDataService::Instance().addOrReplace("group1", group);
    return group;
  }

  /// Helper to fabricate a workspace group consisting of equal sized
  /// matrixworkspaces. BUT WITHOUT MULTIPERIOD LOGS AT ZERO.
  WorkspaceGroup_sptr create_good_zerod_multiperiod_workspace_group() {
    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    // a->setName("a2");
    // b->setName("b2");
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    // group->setName("group2");
    group->addWorkspace(a);
    group->addWorkspace(b); // No multiperiod logs added.
    const bool calculateNPeriods = false;
    const int nPeriods = 0;
    add_periods_logs(group, calculateNPeriods, nPeriods);
    // the two workspaces get same name?
    //    AnalysisDataService::Instance().addOrReplace(a->name(), a);
    //    AnalysisDataService::Instance().addOrReplace(a->name(), b);
    AnalysisDataService::Instance().addOrReplace("a2", a);
    AnalysisDataService::Instance().addOrReplace("b2", b);
    AnalysisDataService::Instance().addOrReplace("group2", group);
    return group;
  }

  /// Helper to fabricate a workspace group with two workspaces, but nperiods =
  /// 5
  WorkspaceGroup_sptr create_corrupted_multiperiod_workspace_group() {
    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    // a->setName("a4");
    // b->setName("b4");
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    // group->setName("group4");
    group->addWorkspace(a);
    group->addWorkspace(b); // No multiperiod logs added.
    const bool calculateNPeriods = false;
    const int nPeriods = 5;
    add_periods_logs(group, calculateNPeriods, nPeriods);
    // the two workspaces get same name?
    //    AnalysisDataService::Instance().addOrReplace(a->name(), a);
    //    AnalysisDataService::Instance().addOrReplace(a->name(), b);
    AnalysisDataService::Instance().addOrReplace("a4", a);
    AnalysisDataService::Instance().addOrReplace("b4", b);
    AnalysisDataService::Instance().addOrReplace("group4", group);
    return group;
  }

  /// Helper to fabricate a workspace group consisting of equal sized
  /// matrixworkspaces.
  WorkspaceGroup_sptr create_good_multiperiod_workspace_group() {
    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    // a->setName("a3");
    // b->setName("b3");
    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    // group->setName("group3");
    group->addWorkspace(a);
    group->addWorkspace(b);
    add_periods_logs(group);
    AnalysisDataService::Instance().addOrReplace("a3", a);
    AnalysisDataService::Instance().addOrReplace("b3", b);
    AnalysisDataService::Instance().addOrReplace("group3", group);
    return group;
  }

  template <typename T>
  WorkspaceGroup_sptr create_group_workspace_with_sample_logs(
      const std::string &merge_type, const std::string &merge_list,
      const T &value_1, const T &value_2, const T &value_3, const T &value_4,
      const std::string &tolerances = "") {
    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 1000,
                                                                     true);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 1000,
                                                                     true);

    Property *prop1 = new PropertyWithValue<T>("prop1", value_1);
    Property *prop2 = new PropertyWithValue<T>("prop1", value_2);
    Property *prop3 = new PropertyWithValue<T>("prop2", value_3);
    Property *prop4 = new PropertyWithValue<T>("prop2", value_4);

    a->mutableRun().addLogData(prop1);
    b->mutableRun().addLogData(prop2);
    a->mutableRun().addLogData(prop3);
    b->mutableRun().addLogData(prop4);

    // add start times
    Property *time1 =
        new PropertyWithValue<std::string>("start_time", "2013-06-25T10:59:15");
    Property *time2 =
        new PropertyWithValue<std::string>("start_time", "2013-06-25T11:59:15");
    a->mutableRun().addLogData(time1);
    b->mutableRun().addLogData(time2);

    a->instrumentParameters().addString(a->getInstrument()->getComponentID(),
                                        merge_type, merge_list);
    b->instrumentParameters().addString(b->getInstrument()->getComponentID(),
                                        merge_type, merge_list);

    // add tolerances
    a->instrumentParameters().addString(
        a->getInstrument()->getComponentID(),
        MergeRunsParameter::FAIL_MERGE_TOLERANCES, tolerances);
    b->instrumentParameters().addString(
        b->getInstrument()->getComponentID(),
        MergeRunsParameter::FAIL_MERGE_TOLERANCES, tolerances);

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    group->addWorkspace(b);

    AnalysisDataService::Instance().addOrReplace("a1", a);
    AnalysisDataService::Instance().addOrReplace("b1", b);
    AnalysisDataService::Instance().addOrReplace("group1", group);
    return group;
  }

  WorkspaceGroup_sptr create_group_detector_scan_workspaces(
      size_t nTimeIndexes = 2, size_t startTimeForSecondWorkspace = 0) {
    const int N_HIST = 2;
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::
        create2DDetectorScanWorkspaceWithFullInstrument(N_HIST, 1000,
                                                        nTimeIndexes, 0);
    MatrixWorkspace_sptr b = WorkspaceCreationHelper::
        create2DDetectorScanWorkspaceWithFullInstrument(
            N_HIST, 1000, nTimeIndexes, startTimeForSecondWorkspace);

    // Change the values in the histogram for the workspaces, so we can do
    // some extra checks
    for (size_t i = 0; i < a->getNumberHistograms(); ++i) {
      auto histogram = a->histogram(i);
      auto &counts = histogram.mutableY();
      std::transform(counts.begin(), counts.end(), counts.begin(),
                     [](double count) { return count + 1; });
      a->setHistogram(i, histogram);
    }
    for (size_t i = 0; i < b->getNumberHistograms(); ++i) {
      auto histogram = b->histogram(i);
      auto &counts = histogram.mutableY();
      std::transform(counts.begin(), counts.end(), counts.begin(),
                     [](double count) { return count + 2; });
      b->setHistogram(i, histogram);
    }

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    group->addWorkspace(b);

    AnalysisDataService::Instance().addOrReplace("a1", a);
    AnalysisDataService::Instance().addOrReplace("b1", b);
    AnalysisDataService::Instance().addOrReplace("group1", group);
    return group;
  }

  template <typename T>
  MatrixWorkspace_sptr create_workspace_with_sample_logs(
      const std::string &merge_type, const std::string &merge_list,
      const T &value_1, const T &value_2, const std::string &tolerances = "") {
    MatrixWorkspace_sptr c =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(2, 1000,
                                                                     true);

    Property *prop5 = new PropertyWithValue<T>("prop1", value_1);
    Property *prop6 = new PropertyWithValue<T>("prop2", value_2);

    c->mutableRun().addLogData(prop5);
    c->mutableRun().addLogData(prop6);

    // add start times
    Property *time3 =
        new PropertyWithValue<std::string>("start_time", "2013-06-25T12:59:15");
    c->mutableRun().addLogData(time3);

    c->instrumentParameters().addString(c->getInstrument()->getComponentID(),
                                        merge_type, merge_list);

    c->instrumentParameters().addString(
        c->getInstrument()->getComponentID(),
        MergeRunsParameter::FAIL_MERGE_TOLERANCES, tolerances);

    AnalysisDataService::Instance().addOrReplace("c1", c);

    return c;
  }

  void do_test_treat_as_non_period_groups(WorkspaceGroup_sptr input) {
    MatrixWorkspace_sptr sampleInputWorkspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(input->getItem(0));
    const double uniformSignal = sampleInputWorkspace->y(0)[0];
    const double uniformError = sampleInputWorkspace->e(0)[0];
    const size_t nXValues = sampleInputWorkspace->x(0).size();

    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspaces", input->getName() + "," + input->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr wsOut = Mantid::API::AnalysisDataService::Instance()
                                     .retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT(wsOut != nullptr);
    for (size_t j = 0; j < wsOut->getNumberHistograms(); ++j) {
      using Mantid::MantidVec;
      const auto &xValues = wsOut->x(j);
      const auto &yValues = wsOut->y(j);
      const auto &eValues = wsOut->e(j);
      TS_ASSERT_EQUALS(nXValues, xValues.size());
      // Loop through each y-value in the histogram
      for (size_t k = 0; k < yValues.size(); ++k) {
        TS_ASSERT_EQUALS(4 * uniformSignal, yValues[k]);
        TS_ASSERT_DELTA(std::sqrt(4 * uniformError * uniformError), eValues[k],
                        0.0001);
      }
    }
  }

public:
  static MergeRunsTest *createSuite() { return new MergeRunsTest(); }
  static void destroySuite(MergeRunsTest *suite) { delete suite; }

  EventWorkspace_sptr ev1, ev2, ev3, ev4, ev5, ev6, evg1, evg2, evg3;

  MergeRunsTest() {
    AnalysisDataService::Instance().add(
        "in1", WorkspaceCreationHelper::create2DWorkspaceBinned(3, 10, 1.));
    AnalysisDataService::Instance().add(
        "in2", WorkspaceCreationHelper::create2DWorkspaceBinned(3, 10, 1.));
    AnalysisDataService::Instance().add(
        "in3", WorkspaceCreationHelper::create2DWorkspaceBinned(3, 10, 1.));
    AnalysisDataService::Instance().add(
        "in4", WorkspaceCreationHelper::create2DWorkspaceBinned(3, 5, 20.));
    AnalysisDataService::Instance().add(
        "in5", WorkspaceCreationHelper::create2DWorkspaceBinned(3, 5, 3.5, 2.));
    AnalysisDataService::Instance().add(
        "in6", WorkspaceCreationHelper::create2DWorkspaceBinned(3, 3, 2., 2.));
  }

  void checkOutput(std::string wsname) {
    EventWorkspace_sptr output;
    TimeSeriesProperty<double> *log;
    int log1, log2, logTot;
    size_t nev1, nev2, nevTot;
    double pc1, pc2, pcTot;

    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "cncs1");)
    log = dynamic_cast<TimeSeriesProperty<double> *>(
        output->run().getProperty("proton_charge"));
    log1 = log->realSize();
    nev1 = output->getNumberEvents();
    pc1 = output->run().getProtonCharge();

    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "cncs2");)
    log = dynamic_cast<TimeSeriesProperty<double> *>(
        output->run().getProperty("proton_charge"));
    log2 = log->realSize();
    nev2 = output->getNumberEvents();
    pc2 = output->run().getProtonCharge();

    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsname));
    TS_ASSERT(output);

    // This many pixels total at CNCS
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 51200);

    log = dynamic_cast<TimeSeriesProperty<double> *>(
        output->run().getProperty("proton_charge"));
    logTot = log->realSize();
    nevTot = output->getNumberEvents();
    pcTot = output->run().getProtonCharge();

    // Total # of log entries
    TS_ASSERT_EQUALS(logTot, log1 + log2);
    // Summed up the proton charge
    TS_ASSERT_EQUALS(pcTot, pc1 + pc2);
    // Total events counted.
    TS_ASSERT_EQUALS(nevTot, nev1 + nev2);
  }

  std::vector<int> makeVector(int num, ...) {
    std::vector<int> retVal;
    va_list vl;
    va_start(vl, num);
    for (int i = 0; i < num; i++)
      retVal.push_back(va_arg(vl, int));
    return retVal;
  }

  void EventSetup() {
    ev1 =
        WorkspaceCreationHelper::createEventWorkspace(3, 10, 100, 0.0, 1.0, 3);
    AnalysisDataService::Instance().addOrReplace(
        "ev1", boost::dynamic_pointer_cast<MatrixWorkspace>(ev1)); // 100 ev
    AnalysisDataService::Instance().addOrReplace(
        "ev2", boost::dynamic_pointer_cast<MatrixWorkspace>(
                   WorkspaceCreationHelper::createEventWorkspace(
                       3, 10, 100, 0.0, 1.0, 2))); // 200 ev
    AnalysisDataService::Instance().addOrReplace(
        "ev3", boost::dynamic_pointer_cast<MatrixWorkspace>(
                   WorkspaceCreationHelper::createEventWorkspace(
                       3, 10, 100, 0.0, 1.0, 2, 100))); // 200 events per
                                                        // spectrum, but the
                                                        // spectra are at
                                                        // different pixel ids
    // Make one with weird units
    MatrixWorkspace_sptr ev4 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::createEventWorkspace(3, 10, 100, 0.0, 1.0, 2,
                                                      100));
    ev4->setYUnit("Microfurlongs per Megafortnights");
    AnalysisDataService::Instance().addOrReplace("ev4_weird_units", ev4);
    AnalysisDataService::Instance().addOrReplace(
        "ev5", boost::dynamic_pointer_cast<MatrixWorkspace>(
                   WorkspaceCreationHelper::createEventWorkspace(
                       5, 10, 100, 0.0, 1.0, 2, 100))); // 200 events per
                                                        // spectrum, but the
                                                        // spectra are at
                                                        // different pixel ids
    ev6 = WorkspaceCreationHelper::createEventWorkspace(6, 10, 100, 0.0, 1.0,
                                                        3); // ids 0-5
    AnalysisDataService::Instance().addOrReplace(
        "ev6", boost::dynamic_pointer_cast<MatrixWorkspace>(ev6));
    // a 2d workspace with the value 2 in each bin
    AnalysisDataService::Instance().addOrReplace(
        "in2D",
        WorkspaceCreationHelper::create2DWorkspaceBinned(3, 10, 0.0, 1.0));

    std::vector<std::vector<int>> groups;

    groups.clear();
    groups.push_back(makeVector(3, 0, 1, 2));
    groups.push_back(makeVector(3, 3, 4, 5));
    evg1 = WorkspaceCreationHelper::createGroupedEventWorkspace(groups, 100);
    AnalysisDataService::Instance().addOrReplace(
        "evg1", boost::dynamic_pointer_cast<MatrixWorkspace>(evg1));

    // let's check on the setup
    TS_ASSERT_EQUALS(evg1->getNumberEvents(), 600);
    TS_ASSERT_EQUALS(evg1->getNumberHistograms(), 2);
    TS_ASSERT(evg1->getSpectrum(0).hasDetectorID(0));
    TS_ASSERT(evg1->getSpectrum(0).hasDetectorID(1));
    TS_ASSERT(evg1->getSpectrum(0).hasDetectorID(2));
    TS_ASSERT(evg1->getSpectrum(1).hasDetectorID(3));

    groups.clear();
    groups.push_back(makeVector(2, 3, 4));
    groups.push_back(makeVector(3, 0, 1, 2));
    groups.push_back(makeVector(1, 15));
    evg2 = WorkspaceCreationHelper::createGroupedEventWorkspace(groups, 100);
    AnalysisDataService::Instance().addOrReplace(
        "evg2", boost::dynamic_pointer_cast<MatrixWorkspace>(evg2));
  }

  void EventTeardown() {
    AnalysisDataService::Instance().remove("ev1");
    AnalysisDataService::Instance().remove("ev2");
    AnalysisDataService::Instance().remove("ev3");
    AnalysisDataService::Instance().remove("ev4_weird_units");
    AnalysisDataService::Instance().remove("ev5");
    AnalysisDataService::Instance().remove("ev6");
    AnalysisDataService::Instance().remove("in2D");
    AnalysisDataService::Instance().remove("evg1");
    AnalysisDataService::Instance().remove("evOUT");
    AnalysisDataService::Instance().remove("out2D");
  }

  void testTheBasics() {
    TS_ASSERT_EQUALS(merge.name(), "MergeRuns");
    TS_ASSERT_EQUALS(merge.version(), 1);
  }

  void testInit() {
    TS_ASSERT_THROWS_NOTHING(merge.initialize());
    TS_ASSERT(merge.isInitialized());
  }

  //-----------------------------------------------------------------------------------------------
  void testExec() {
    if (!merge.isInitialized())
      merge.initialize();

    TS_ASSERT_THROWS_NOTHING(
        merge.setPropertyValue("InputWorkspaces", "in1,in2,in3"));
    TS_ASSERT_THROWS_NOTHING(
        merge.setPropertyValue("OutputWorkspace", "outWS"));

    TS_ASSERT_THROWS_NOTHING(merge.execute());
    TS_ASSERT(merge.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outWS"));
    MatrixWorkspace_const_sptr input =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("in1");

    TS_ASSERT_EQUALS(input->getNumberHistograms(),
                     output->getNumberHistograms());
    TS_ASSERT_EQUALS(input->blocksize(), output->blocksize());

    const size_t xsize = output->blocksize();
    for (size_t i = 0; i < output->getNumberHistograms(); ++i) {
      const auto &outX = output->x(i);
      const auto &outY = output->y(i);
      const auto &outE = output->e(i);
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], input->x(i)[j], 1e-12);
        TS_ASSERT_DELTA(outY[j], 6.0, 1e-12);
        TS_ASSERT_DELTA(outE[j], sqrt(6.0), 1e-5);
      }
    }

    AnalysisDataService::Instance().remove("outWS");
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_MixingEventAnd2D_gives_a2D() {
    EventSetup();
    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "ev1,ev2,in1");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());
    // Not an EventWorkspace
    EventWorkspace_sptr outEvent =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    TS_ASSERT(!outEvent);
    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MixedIDs() {
    EventSetup();
    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "ev1,ev2,ev3");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Should have 300+600+600 = 1500 total events
    TS_ASSERT_EQUALS(output->getNumberEvents(), 1500);
    // 6 unique pixel ids
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 6);

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MismatchedUnits_fail() {
    EventSetup();
    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "ev1,ev4_weird_units,ev3");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(!mrg.isExecuted());
    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MatchingPixelIDs() {
    EventSetup();
    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "ev1,ev2");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Should have 300+600
    TS_ASSERT_EQUALS(output->getNumberEvents(), 900);
    // 3 unique pixel ids
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 3);

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_MatchingPixelIDs_WithWorkspaceGroup() {
    EventSetup();

    GroupWorkspaces grpwsalg;
    grpwsalg.initialize();
    std::vector<std::string> input{"ev1", "ev2"};
    TS_ASSERT_THROWS_NOTHING(grpwsalg.setProperty("InputWorkspaces", input));
    TS_ASSERT_THROWS_NOTHING(
        grpwsalg.setProperty("OutputWorkspace", "ev1_and_ev2_workspace_group"));
    TS_ASSERT_THROWS_NOTHING(grpwsalg.execute());
    TS_ASSERT(grpwsalg.isExecuted());

    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "ev1_and_ev2_workspace_group");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Should have 300+600
    TS_ASSERT_EQUALS(output->getNumberEvents(), 900);
    // 3 unique pixel ids
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 3);

    AnalysisDataService::Instance().remove("ev1_and_ev2_workspace_group");
    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped1() {
    EventSetup();

    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "evg1,ev1");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Total # of events
    TS_ASSERT_EQUALS(output->getNumberEvents(),
                     ev1->getNumberEvents() + evg1->getNumberEvents());
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 2); // 2 groups; 0-2 and 3-5

    TS_ASSERT_EQUALS(output->getSpectrum(0).getNumberEvents(),
                     600); // 300 + 3x100
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(0));
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(1));
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(2));

    TS_ASSERT_EQUALS(output->getSpectrum(1).getNumberEvents(), 300); // 300
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(3));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(4));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(5));

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped1_flipped() {
    EventSetup();

    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "ev1,evg1");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Total # of events
    TS_ASSERT_EQUALS(output->getNumberEvents(),
                     ev1->getNumberEvents() + evg1->getNumberEvents());
    // Grouped pixel IDs: 0; 1; 2; 012; 345
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 5);
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(0));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(1));
    TS_ASSERT(output->getSpectrum(2).hasDetectorID(2));
    TS_ASSERT(output->getSpectrum(3).hasDetectorID(0));
    TS_ASSERT(output->getSpectrum(3).hasDetectorID(1));
    TS_ASSERT(output->getSpectrum(3).hasDetectorID(2));
    TS_ASSERT(output->getSpectrum(4).hasDetectorID(3));
    TS_ASSERT(output->getSpectrum(4).hasDetectorID(4));
    TS_ASSERT(output->getSpectrum(4).hasDetectorID(5));

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped2() {
    EventSetup();

    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "evg2,ev6");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Total # of events
    TS_ASSERT_EQUALS(output->getNumberEvents(),
                     ev6->getNumberEvents() + evg2->getNumberEvents());
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 4);
    TS_ASSERT_EQUALS(output->getSpectrum(0).getNumberEvents(),
                     400); // 4 lists were added
    TS_ASSERT_EQUALS(output->getSpectrum(1).getNumberEvents(), 600);
    TS_ASSERT_EQUALS(output->getSpectrum(2).getNumberEvents(), 100);
    TS_ASSERT_EQUALS(output->getSpectrum(3).getNumberEvents(), 100);
    // Groups are 3,4;   0,1,2;   15(from ev6); 5(unused in ev6)
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(3));
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(4));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(0));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(1));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(2));
    TS_ASSERT(output->getSpectrum(2).hasDetectorID(15));
    TS_ASSERT(output->getSpectrum(3).hasDetectorID(
        5)); // Leftover from the ev1 workspace

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testExec_Events_Grouped3() {
    EventSetup();

    MergeRuns mrg;
    mrg.initialize();
    mrg.setPropertyValue("InputWorkspaces", "evg1,ev1,evg2");
    mrg.setPropertyValue("OutputWorkspace", "outWS");
    mrg.execute();
    TS_ASSERT(mrg.isExecuted());

    // Get the output event workspace
    EventWorkspace_const_sptr output;
    output =
        AnalysisDataService::Instance().retrieveWS<EventWorkspace>("outWS");
    // This checks that it is indeed an EW
    TS_ASSERT(output);

    // Total # of events
    TS_ASSERT_EQUALS(output->getNumberEvents(), ev1->getNumberEvents() +
                                                    evg1->getNumberEvents() +
                                                    evg2->getNumberEvents());
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 3);

    TS_ASSERT_EQUALS(output->getSpectrum(0).getNumberEvents(),
                     900); // 300 (evg1) + 3x100 (ev1) + 3x100 (evg2 had 012)
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(0));
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(1));
    TS_ASSERT(output->getSpectrum(0).hasDetectorID(2));

    TS_ASSERT_EQUALS(output->getSpectrum(1).getNumberEvents(),
                     500); // 300 + 2x100 (evg2 had 3,4 only)
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(3));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(4));
    TS_ASSERT(output->getSpectrum(1).hasDetectorID(5));

    // Leftover 15 from evg2
    TS_ASSERT_EQUALS(output->getSpectrum(2).getNumberEvents(), 100); // (evg2)
    TS_ASSERT(output->getSpectrum(2).hasDetectorID(15));

    EventTeardown();
  }

  //-----------------------------------------------------------------------------------------------
  void testInvalidInputs() {
    MergeRuns merge2;
    TS_ASSERT_THROWS_NOTHING(merge2.initialize());
    TS_ASSERT_THROWS_NOTHING(merge.setPropertyValue("OutputWorkspace", "null"));
    TS_ASSERT_THROWS(merge2.execute(), std::runtime_error);
    TS_ASSERT(!merge2.isExecuted());
    MatrixWorkspace_sptr badIn =
        WorkspaceCreationHelper::create2DWorkspace123(3, 10, 1);
    badIn->mutableX(0) = 2.0;
    AnalysisDataService::Instance().add("badIn", badIn);
    TS_ASSERT_THROWS_ANYTHING(
        merge.setPropertyValue("InputWorkspaces", "ws1,badIn"));
  }

  //-----------------------------------------------------------------------------------------------
  void testNonOverlapping() {
    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "in1,in4"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outer"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outer"));

    auto &X = output->x(0);
    TS_ASSERT_EQUALS(X.size(), 17);
    int i;
    for (i = 0; i < 11; ++i) {
      TS_ASSERT_EQUALS(X[i], i + 1);
    }
    for (; i < 17; ++i) {
      TS_ASSERT_EQUALS(X[i], i + 9);
    }

    AnalysisDataService::Instance().remove("outer");
  }

  //-----------------------------------------------------------------------------------------------
  void testIntersection() {
    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "in1,in5"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outer"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outer"));

    const auto &X = output->x(0);
    TS_ASSERT_EQUALS(X.size(), 8);
    int i;
    for (i = 0; i < 3; ++i) {
      TS_ASSERT_EQUALS(X[i], i + 1);
    }
    for (; i < 8; ++i) {
      TS_ASSERT_EQUALS(X[i], 2 * i - 0.5);
    }

    AnalysisDataService::Instance().remove("outer");
  }

  //-----------------------------------------------------------------------------------------------
  void testInclusion() {
    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "in6,in1"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outer"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    MatrixWorkspace_const_sptr output;
    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outer"));

    const auto &X = output->x(0);
    TS_ASSERT_EQUALS(X.size(), 8);
    int i;
    for (i = 0; i < 2; ++i) {
      TS_ASSERT_EQUALS(X[i], i + 1);
    }
    for (; i < 5; ++i) {
      TS_ASSERT_EQUALS(X[i], 2 * i);
    }
    for (; i < 8; ++i) {
      TS_ASSERT_EQUALS(X[i], i + 4);
    }

    AnalysisDataService::Instance().remove("outer");
  }

  void do_test_validation_throws(WorkspaceGroup_sptr a, WorkspaceGroup_sptr b) {
    MergeRuns alg;
    alg.setRethrows(true);
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", a->getName() + "," + b->getName());
    alg.setPropertyValue("OutputWorkspace", "out");
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
  }

  void test_mixed_multiperiod_group_and_non_multiperiod_group_inputs_throws() {
    WorkspaceGroup_sptr a = create_good_workspace_group();
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group();
    do_test_validation_throws(a, b);
  }

  void test_throws_if_multiperiod_input_nperiods_corrupted() {
    WorkspaceGroup_sptr a = create_corrupted_multiperiod_workspace_group();
    WorkspaceGroup_sptr b = create_good_multiperiod_workspace_group();
    do_test_validation_throws(a, b);
  }

  void test_throws_if_workspace_ordering_in_group_corrupted() {
    WorkspaceGroup_sptr a = create_good_multiperiod_workspace_group();
    MatrixWorkspace_sptr first = boost::dynamic_pointer_cast<MatrixWorkspace>(
        a->getItem(0)); // Has current_period = 1
    MatrixWorkspace_sptr second = boost::dynamic_pointer_cast<MatrixWorkspace>(
        a->getItem(1)); // Has current_period = 2
    WorkspaceGroup_sptr aCorrupted = boost::make_shared<WorkspaceGroup>();
    aCorrupted->addWorkspace(second);
    aCorrupted->addWorkspace(first);
    // aCorrupted->setName("aCorrupted");
    Mantid::API::AnalysisDataService::Instance().addOrReplace("aCorrupted",
                                                              aCorrupted);

    do_test_validation_throws(aCorrupted, a);
  }

  void do_test_with_multiperiod_data(WorkspaceGroup_sptr input) {
    // Extract some internal information from the nested workspaces in order to
    // run test asserts later.
    const size_t expectedNumHistograms =
        boost::dynamic_pointer_cast<MatrixWorkspace>(input->getItem(0))
            ->getNumberHistograms();
    MatrixWorkspace_sptr sampleNestedInputWorkspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(input->getItem(0));
    const double uniformSignal = sampleNestedInputWorkspace->y(0)[0];
    const double uniformError = sampleNestedInputWorkspace->e(0)[0];
    const size_t nXValues = sampleNestedInputWorkspace->x(0).size();

    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspaces", input->getName() + "," + input->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outer"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    WorkspaceGroup_sptr wsgroup =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "outer");
    TS_ASSERT(wsgroup != nullptr);
    TS_ASSERT_EQUALS(input->size(), wsgroup->size());
    // Loop through each workspace in the group
    for (size_t i = 0; i < wsgroup->size(); ++i) {
      MatrixWorkspace_sptr ws =
          boost::dynamic_pointer_cast<MatrixWorkspace>(wsgroup->getItem(i));
      TS_ASSERT(ws != nullptr);
      TS_ASSERT_EQUALS(expectedNumHistograms, ws->getNumberHistograms());
      // Loop through each histogram in each workspace
      for (size_t j = 0; j < ws->getNumberHistograms(); ++j) {
        using Mantid::MantidVec;
        const auto &xValues = ws->x(j);
        const auto &yValues = ws->y(j);
        const auto &eValues = ws->e(j);
        TS_ASSERT_EQUALS(nXValues, xValues.size());
        // Loop through each y-value in the histogram
        for (size_t k = 0; k < yValues.size(); ++k) {
          TS_ASSERT_EQUALS(2 * uniformSignal, yValues[k]);
          TS_ASSERT_DELTA(std::sqrt(2 * uniformError * uniformError),
                          eValues[k], 0.0001);
        }
      }
    }
  }

  void test_with_zerod_nperiods_logs() {
    // Creates a NON-MULIPERIOD workspace group containing two identical matrix
    // workspaces with uniform signal and error, and zerod n_period logs on all
    // workspaces.
    WorkspaceGroup_sptr input = create_good_zerod_multiperiod_workspace_group();
    do_test_treat_as_non_period_groups(input);
  }

  void test_with_missing_nperiods_logs() {
    // Creates a NON-MULIPERIOD workspace group containing two identical matrix
    // workspaces with uniform signal and error, and No n_period logs on all
    // workspaces.
    WorkspaceGroup_sptr input = create_good_workspace_group();
    do_test_treat_as_non_period_groups(input);
  }

  void test_with_multiperiod_data() {
    WorkspaceGroup_sptr input = create_good_multiperiod_workspace_group();
    do_test_with_multiperiod_data(input);
  }

  void test_useCustomInputPropertyName() {
    MergeRuns alg;
    TS_ASSERT(alg.useCustomInputPropertyName());
  }

  void do_test_mergeSampleLogs(const WorkspaceGroup_sptr &input,
                               const std::string &propertyName,
                               const std::string &mergeType,
                               const std::string &result, const int filesMerged,
                               const bool noOutput = false) {
    MergeRuns alg;
    alg.initialize();

    do_test_mergeSampleLogs_modified_alg(alg, input, propertyName, mergeType,
                                         result, filesMerged, noOutput);
  }

  void do_test_mergeSampleLogs_modified_alg(MergeRuns &alg,
                                            const WorkspaceGroup_sptr &input,
                                            const std::string &propertyName,
                                            const std::string &mergeType,
                                            const std::string &result,
                                            const int filesMerged,
                                            const bool noOutput = false) {

    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", input->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outWS"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr output;
    if (noOutput) {
      TS_ASSERT_THROWS(
          AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"),
          Mantid::Kernel::Exception::NotFoundError);
      return;
    } else {
      TS_ASSERT_THROWS_NOTHING(
          output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              "outWS"));
    }

    Property *prop;

    TS_ASSERT_EQUALS(output->y(0).front(), 2.0 * filesMerged);

    if (mergeType.compare(MergeRunsParameter::TIME_SERIES_MERGE) == 0) {
      prop = output->run().getTimeSeriesProperty<double>(propertyName);
      TS_ASSERT_EQUALS(prop->value(), result);
    } else {
      prop = output->run().getLogData(propertyName);
      TS_ASSERT_EQUALS(prop->value(), result);
    }

    // We add and remove the property from the addee workspace to supress a
    // warning. Check it is back here.
    const auto &addeeWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("b1");
    TS_ASSERT_THROWS_NOTHING(addeeWS->run().getProperty("prop1"));

    sample_logs_teardown();
  }

  void sample_logs_teardown() {
    AnalysisDataService::Instance().remove("a1");
    AnalysisDataService::Instance().remove("b1");
    AnalysisDataService::Instance().remove("c1");
    AnalysisDataService::Instance().remove("group1");
    AnalysisDataService::Instance().remove("outWS");
    AnalysisDataService::Instance().remove("outWS1");
    AnalysisDataService::Instance().remove("outWS2");
  }

  void test_mergeSampleLogs_sum() {
    const std::string mergeType = MergeRunsParameter::SUM_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.2, 2.3, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "3.5", 2);
  }

  void test_mergeSampleLogs_time_series() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    do_test_mergeSampleLogs(
        ws, "prop1", mergeType,
        "2013-Jun-25 10:59:15  1\n2013-Jun-25 11:59:15  2\n", 2);
  }

  void test_mergeSampleLogs_time_series_multiple() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1, prop2", 1.0, 2.0, 3.0, 4.0);
    do_test_mergeSampleLogs(
        ws, "prop2", mergeType,
        "2013-Jun-25 10:59:15  3\n2013-Jun-25 11:59:15  4\n", 2);
  }

  void test_mergeSampleLogs_list() {
    const std::string mergeType = MergeRunsParameter::LIST_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1, 2", 2);
  }

  void test_mergeSampleLogs_warn() {
    const std::string mergeType = MergeRunsParameter::WARN_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 2);
  }

  void test_mergeSampleLogs_fail_where_params_are_equal_succeeds() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 1.0, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 2);
    do_test_mergeSampleLogs(create_group_workspace_with_sample_logs<int>(
                                mergeType, "prop1", 1, 1, 2, 2),
                            "prop1", mergeType, "1", 2);
  }

  void test_mergeSampleLogs_fail_where_params_are_different_fails() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 1);
  }

  void
  test_mergeSampleLogs_fail_where_params_are_different_but_inside_tolerance_succeeds() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0, "2.0");
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 2);
  }

  void
  test_mergeSampleLogs_fail_where_params_are_different_but_outside_tolerance_fails() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0, "0.5");
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 1);
  }

  void
  test_mergeSampleLogs_fail_where_params_with_one_outside_tolerance_fails_multiple_tolerances() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1, prop2", 1.0, 2.0, 3.0, 4.0, "0.5, 1.5");
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 1);
  }

  void
  test_mergeSampleLogs_fail_where_params_with_both_tolerances_outside_fails() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1, prop2", 1.0, 2.0, 3.0, 4.0, "0.5");
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 1);
  }

  void test_mergeSampleLogs_fail() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 1);
    ws = create_group_workspace_with_sample_logs<double>(mergeType, "prop1",
                                                         1.0, 1.0, 0.0, 0.0);
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 2);
  }

  void test_mergeSampleLogs_non_existent_log_is_ignored() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;
    WorkspaceGroup_sptr gws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    MatrixWorkspace_sptr a =
        boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    a->instrumentParameters().addString(a->getInstrument()->getComponentID(),
                                        mergeType, "prop1, non_existent");
    do_test_mergeSampleLogs(
        gws, "prop1", mergeType,
        "2013-Jun-25 10:59:15  1\n2013-Jun-25 11:59:15  2\n", 2);
  }

  void test_mergeSampleLogs_log_used_twice_with_same_merge_type_throws_error() {
    const std::string mergeTypeTimeSeries =
        MergeRunsParameter::TIME_SERIES_MERGE;
    WorkspaceGroup_sptr gws = create_group_workspace_with_sample_logs<double>(
        mergeTypeTimeSeries, "prop1", 1.0, 2.0, 0.0, 0.0);

    MatrixWorkspace_sptr a =
        boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    a->instrumentParameters().addString(a->getInstrument()->getComponentID(),
                                        mergeTypeTimeSeries, "prop1, prop1");

    // Error is caught by Algorithm, but check no output workspace created
    do_test_mergeSampleLogs(gws, "prop1", mergeTypeTimeSeries, "", 1, true);
  }

  void
  test_mergeSampleLogs_log_used_twice_with_incompatible_merge_types_fails() {
    const std::string mergeTypeTimeSeries =
        MergeRunsParameter::TIME_SERIES_MERGE;
    const std::string mergeTypeList = MergeRunsParameter::LIST_MERGE;
    WorkspaceGroup_sptr gws = create_group_workspace_with_sample_logs<double>(
        mergeTypeTimeSeries, "prop1", 1.0, 2.0, 0.0, 0.0);
    MatrixWorkspace_sptr a =
        boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    a->instrumentParameters().addString(a->getInstrument()->getComponentID(),
                                        mergeTypeList, "prop1");

    // Error is caught by Algorithm, but check no output workspace created
    do_test_mergeSampleLogs(
        gws, "prop1", mergeTypeTimeSeries,
        "2013-Jun-25 10:59:15  1\n2013-Jun-25 11:59:15  2\n", 2, true);
  }

  void
  test_mergeSampleLogs_log_used_twice_with_compatible_merge_types_suceeds() {
    const std::string mergeTypeTimeSeries =
        MergeRunsParameter::TIME_SERIES_MERGE;
    const std::string mergeTypeWarn = MergeRunsParameter::WARN_MERGE;
    WorkspaceGroup_sptr gws = create_group_workspace_with_sample_logs<double>(
        mergeTypeTimeSeries, "prop1", 1.0, 2.0, 0.0, 0.0);
    MatrixWorkspace_sptr a =
        boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    a->instrumentParameters().addString(a->getInstrument()->getComponentID(),
                                        mergeTypeWarn, "prop1");

    // Error is caught by Algorithm, but check no output workspace created
    do_test_mergeSampleLogs(
        gws, "prop1", mergeTypeTimeSeries,
        "2013-Jun-25 10:59:15  1\n2013-Jun-25 11:59:15  2\n", 2);
  }

  void test_mergeSampleLogs_non_numeric_property_fails_to_merge() {
    const std::string mergeType = MergeRunsParameter::SUM_MERGE;
    do_test_mergeSampleLogs(
        create_group_workspace_with_sample_logs<std::string>(
            mergeType, "prop1", "1", "two", "", ""),
        "prop1", mergeType, "1", 1);
  }

  void
  test_mergeSampleLogs_non_numeric_property_in_first_ws_skips_merging_parameter() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;
    auto ws = create_group_workspace_with_sample_logs<std::string>(
        mergeType, "prop1", "one", "two", "", "");
    // should get stuck when trying to get "prop1" as a time series
    TS_ASSERT_THROWS(do_test_mergeSampleLogs(ws, "prop1", mergeType,
                                             "2013-Jun-25 10:59:15  1\n", 2),
                     std::invalid_argument);
  }

  void test_mergeSampleLogs_with_additional_time_series_property() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::TIME_SERIES_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsTimeSeries", "prop2");
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop2", MergeRunsParameter::TIME_SERIES_MERGE,
        "2013-Jun-25 10:59:15  3\n2013-Jun-25 11:59:15  4\n", 2);
  }

  void test_mergeSampleLogs_with_additional_list_property() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::TIME_SERIES_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsList", "prop2");
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop2", MergeRunsParameter::LIST_MERGE, "3, 4", 2);
  }

  void test_mergeSampleLogs_with_additional_warn_property() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::TIME_SERIES_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsWarn", "prop2");
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop2", MergeRunsParameter::WARN_MERGE, "3", 2);
  }

  void test_mergeSampleLogs_with_additional_fail_property() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::TIME_SERIES_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsFail", "prop2");
    alg.setPropertyValue("SampleLogsFailTolerances", "0.5");
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop2", MergeRunsParameter::FAIL_MERGE, "3", 1);
  }

  void
  test_mergeSampleLogs_time_series_overwriting_in_merge_behaviour_in_algorithm() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0);
    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsList", "prop1");
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop1", MergeRunsParameter::LIST_MERGE, "1, 2", 2);
  }

  void test_mergeSampleLogs_time_series_overwriting_tolerance_in_algorithm() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 0.0, 0.0, "0.5");

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsFail", "prop1");
    alg.setPropertyValue("SampleLogsFailTolerances", "2.0");
    do_test_mergeSampleLogs_modified_alg(alg, ws, "prop1", mergeType, "1", 2);
  }

  void test_mergeSampleLogs_sum_and_error_skips_merging_second_file() {
    auto ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::SUM_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0, "0.5");

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsFail", "prop2");
    alg.setPropertyValue("SampleLogsFailTolerances", "0.5");
    do_test_mergeSampleLogs_modified_alg(alg, ws, "prop1",
                                         MergeRunsParameter::SUM_MERGE, "1", 1);
  }

  void test_mergeSampleLogs_time_series_and_error_skips_merging_second_file() {
    auto ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::TIME_SERIES_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0,
        "0.5");

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsFail", "prop2");
    alg.setPropertyValue("SampleLogsFailTolerances", "0.5");
    do_test_mergeSampleLogs_modified_alg(alg, ws, "prop1",
                                         MergeRunsParameter::TIME_SERIES_MERGE,
                                         "2013-Jun-25 10:59:15  1\n", 1);
  }

  void test_mergeSampleLogs_list_and_error_skips_merging_second_file() {
    auto ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::LIST_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0, "0.5");

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsFail", "prop2");
    alg.setPropertyValue("SampleLogsFailTolerances", "0.5");
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop1", MergeRunsParameter::LIST_MERGE, "1", 1);
  }

  void test_merging_three_workspace_with_time_series() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;

    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 3.0, 4.0);

    MatrixWorkspace_sptr c =
        create_workspace_with_sample_logs<double>(mergeType, "prop1", 5.0, 6.0);
    ws->addWorkspace(c);

    do_test_mergeSampleLogs(ws, "prop1", mergeType,
                            "2013-Jun-25 10:59:15  1\n2013-Jun-25 11:59:15  "
                            "2\n2013-Jun-25 12:59:15  5\n",
                            3);
  }

  void do_test_merge_two_workspaces_then_third(std::string mergeType,
                                               std::string result) {
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr c =
        create_workspace_with_sample_logs<double>(mergeType, "prop1", 5.0, 6.0);
    ws->removeAll();

    MatrixWorkspace_sptr outWS =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS");
    ws->addWorkspace(outWS);
    ws->addWorkspace(c);

    do_test_mergeSampleLogs(ws, "prop1", mergeType, result, 3);
  }

  void test_merging_two_workspace_then_third_with_time_series() {
    do_test_merge_two_workspaces_then_third(
        MergeRunsParameter::TIME_SERIES_MERGE, "2013-Jun-25 10:59:15  "
                                               "1\n2013-Jun-25 11:59:15  "
                                               "2\n2013-Jun-25 12:59:15  5\n");
  }

  void test_merging_two_workspace_then_third_with_list() {
    do_test_merge_two_workspaces_then_third(MergeRunsParameter::LIST_MERGE,
                                            "1, 2, 5");
  }

  void do_test_merging_two_workspaces_both_already_merged(std::string mergeType,
                                                          std::string result) {
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    auto ws2 = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 6.0, 7.0, 8.0, 9.0);

    MergeRuns alg2;
    alg2.initialize();
    alg2.setPropertyValue("InputWorkspaces", ws2->getName());
    alg2.setPropertyValue("OutputWorkspace", "outWS2");
    TS_ASSERT_THROWS_NOTHING(alg2.execute());

    MatrixWorkspace_sptr outWS1 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS1");
    MatrixWorkspace_sptr outWS2 =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS2");
    WorkspaceGroup_sptr groupWS =
        AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>("group1");
    groupWS->removeAll();
    groupWS->addWorkspace(outWS1);
    groupWS->addWorkspace(outWS2);

    do_test_mergeSampleLogs(groupWS, "prop1", mergeType, result, 4);
  }

  void test_merging_two_workspace_then_two_already_merged_with_time_series() {
    do_test_merging_two_workspaces_both_already_merged(
        MergeRunsParameter::TIME_SERIES_MERGE,
        "2013-Jun-25 10:59:15  1\n2013-Jun-25 10:59:15  6\n2013-Jun-25 "
        "11:59:15  2\n2013-Jun-25 11:59:15  7\n");
  }

  void test_merging_two_workspace_then_two_already_merged_with_list() {
    do_test_merging_two_workspaces_both_already_merged(
        MergeRunsParameter::LIST_MERGE, "1, 2, 6, 7");
  }

  void test_merging_single_workspace() {
    const std::string mergeType = MergeRunsParameter::TIME_SERIES_MERGE;

    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 1.0, 2.0, 3.0, 4.0);

    ws->remove("b1");

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS1");
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    do_test_mergeSampleLogs(ws, "prop1", mergeType, "2013-Jun-25 10:59:15  1\n",
                            1);
  }

  void test_mergeSampleLogs_fail_throwing_error() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::TIME_SERIES_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("SampleLogsFail", "prop2");
    alg.setPropertyValue("SampleLogsFailTolerances", "0.5");
    alg.setPropertyValue("FailBehaviour",
                         RunCombinationOptions::STOP_BEHAVIOUR);
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop2", MergeRunsParameter::FAIL_MERGE, "3", 1, true);
  }

  void rebin_one_workspace() {
    Rebin rebinAlg;
    rebinAlg.initialize();
    rebinAlg.setPropertyValue("InputWorkspace", "b1");
    rebinAlg.setPropertyValue("OutputWorkspace", "b1");
    rebinAlg.setPropertyValue("Params", "0.1");
    rebinAlg.execute();
  }

  void test_mergeSampleLogs_with_different_binning_skips_merging() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::SUM_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    rebin_one_workspace();

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("RebinBehaviour",
                         RunCombinationOptions::FAIL_BEHAVIOUR);
    do_test_mergeSampleLogs_modified_alg(alg, ws, "prop1",
                                         MergeRunsParameter::SUM_MERGE, "1", 1);
  }

  void
  test_mergeSampleLogs_with_different_binning_skips_merging_and_throws_error() {
    WorkspaceGroup_sptr ws = create_group_workspace_with_sample_logs<double>(
        MergeRunsParameter::SUM_MERGE, "prop1", 1.0, 2.0, 3.0, 4.0);

    rebin_one_workspace();

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("RebinBehaviour",
                         RunCombinationOptions::FAIL_BEHAVIOUR);
    alg.setPropertyValue("FailBehaviour",
                         RunCombinationOptions::STOP_BEHAVIOUR);
    do_test_mergeSampleLogs_modified_alg(
        alg, ws, "prop1", MergeRunsParameter::SUM_MERGE, "1", 1, true);
  }

  void test_mergeSampleLogs_fail_with_single_negative_tolerance() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1", 0.0, 0.0, 0.0, 0.0, "-1.0");
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 2, true);
  }

  void test_mergeSampleLogs_fail_with_single_negative_tolerance_in_a_list() {
    const std::string mergeType = MergeRunsParameter::FAIL_MERGE;
    auto ws = create_group_workspace_with_sample_logs<double>(
        mergeType, "prop1, prop2", 0.0, 0.0, 0.0, 0.0, "-0.5, 1.5");
    do_test_mergeSampleLogs(ws, "prop1", mergeType, "1", 1, true);
  }

  MatrixWorkspace_sptr
  do_MergeRuns_with_scanning_workspaces(size_t startTime = 0) {
    auto ws = create_group_detector_scan_workspaces(2, startTime);
    MatrixWorkspace_sptr outputWS;

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_NOTHING(alg.execute();)

    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outWS"));

    return outputWS;
  }

  void assert_scan_intervals_are_correct(const DetectorInfo &detInfo,
                                         bool extraTimes = false) {
    const auto TIME_1 = DateAndTime(0, 0);
    const auto TIME_2 = DateAndTime(1, 0);
    const auto TIME_3 = DateAndTime(3, 0);

    const auto PAIR_1 = std::pair<DateAndTime, DateAndTime>(TIME_1, TIME_2);
    const auto PAIR_2 = std::pair<DateAndTime, DateAndTime>(TIME_2, TIME_3);

    TS_ASSERT_EQUALS(detInfo.scanIntervals()[0], PAIR_1)
    TS_ASSERT_EQUALS(detInfo.scanIntervals()[1], PAIR_2)

    if (extraTimes) {
      const auto TIME_4 = DateAndTime(20, 0);
      const auto TIME_5 = DateAndTime(21, 0);
      const auto TIME_6 = DateAndTime(23, 0);

      const auto PAIR_3 = std::pair<DateAndTime, DateAndTime>(TIME_4, TIME_5);
      const auto PAIR_4 = std::pair<DateAndTime, DateAndTime>(TIME_5, TIME_6);

      TS_ASSERT_EQUALS(detInfo.scanIntervals()[2], PAIR_3)
      TS_ASSERT_EQUALS(detInfo.scanIntervals()[3], PAIR_4)
    }
  }

  void assert_scanning_indexing_is_correct(const SpectrumInfo &specInfo,
                                           bool extraSpectra = false) {

    for (size_t i = 0; i < specInfo.size(); ++i) {
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(i).size(), 1)
    }

    const auto SPEC_DEF_1 = std::pair<size_t, size_t>(0, 0);
    const auto SPEC_DEF_2 = std::pair<size_t, size_t>(0, 1);
    const auto SPEC_DEF_3 = std::pair<size_t, size_t>(1, 0);
    const auto SPEC_DEF_4 = std::pair<size_t, size_t>(1, 1);
    TS_ASSERT_EQUALS(specInfo.spectrumDefinition(0)[0], SPEC_DEF_1)
    TS_ASSERT_EQUALS(specInfo.spectrumDefinition(1)[0], SPEC_DEF_2)
    TS_ASSERT_EQUALS(specInfo.spectrumDefinition(2)[0], SPEC_DEF_3)
    TS_ASSERT_EQUALS(specInfo.spectrumDefinition(3)[0], SPEC_DEF_4)

    if (extraSpectra) {
      const auto SPEC_DEF_5 = std::pair<size_t, size_t>(0, 2);
      const auto SPEC_DEF_6 = std::pair<size_t, size_t>(0, 3);
      const auto SPEC_DEF_7 = std::pair<size_t, size_t>(1, 2);
      const auto SPEC_DEF_8 = std::pair<size_t, size_t>(1, 3);
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(4)[0], SPEC_DEF_5)
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(5)[0], SPEC_DEF_6)
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(6)[0], SPEC_DEF_7)
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(7)[0], SPEC_DEF_8)
    } else {
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(4)[0], SPEC_DEF_1)
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(5)[0], SPEC_DEF_2)
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(6)[0], SPEC_DEF_3)
      TS_ASSERT_EQUALS(specInfo.spectrumDefinition(7)[0], SPEC_DEF_4)
    }
  }

  void
  assert_scanning_histograms_correctly_set(const MatrixWorkspace_sptr &ws) {
    TS_ASSERT_EQUALS(ws->histogram(0).y()[0], 1)
    TS_ASSERT_EQUALS(ws->histogram(1).y()[0], 1)
    TS_ASSERT_EQUALS(ws->histogram(2).y()[0], 1)
    TS_ASSERT_EQUALS(ws->histogram(3).y()[0], 1)
    TS_ASSERT_EQUALS(ws->histogram(4).y()[0], 2)
    TS_ASSERT_EQUALS(ws->histogram(5).y()[0], 2)
    TS_ASSERT_EQUALS(ws->histogram(6).y()[0], 2)
    TS_ASSERT_EQUALS(ws->histogram(7).y()[0], 2)
  }

  void
  test_merging_detector_scan_workspaces_with_different_start_times_appends_workspaces() {
    auto outputWS = do_MergeRuns_with_scanning_workspaces(20);

    const auto &detInfo = outputWS->detectorInfo();
    TS_ASSERT_EQUALS(detInfo.size(), 2)
    TS_ASSERT_EQUALS(detInfo.scanCount(), 4)
    assert_scan_intervals_are_correct(detInfo, true);

    const auto &specInfo = outputWS->spectrumInfo();
    TS_ASSERT_EQUALS(specInfo.size(), 8)

    assert_scanning_indexing_is_correct(specInfo, true);
    assert_scanning_histograms_correctly_set(outputWS);
  }

  void
  test_merging_detector_scan_workspaces_with_overlapping_time_intervals_throws() {
    auto ws = create_group_detector_scan_workspaces(2, 1);

    MergeRuns alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: "
                            "scan intervals overlap but not identical")
  }

  void test_merging_detector_scan_workspaces_does_not_append_workspaces() {
    auto outputWS = do_MergeRuns_with_scanning_workspaces();

    TS_ASSERT_EQUALS(outputWS->detectorInfo().size(), 2)
    TS_ASSERT_EQUALS(outputWS->detectorInfo().scanCount(), 2)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 4)

    // Check bins are set correctly
    TS_ASSERT_EQUALS(outputWS->histogram(0).y()[0], 3)
    TS_ASSERT_EQUALS(outputWS->histogram(1).y()[0], 3)
    TS_ASSERT_EQUALS(outputWS->histogram(2).y()[0], 3)
    TS_ASSERT_EQUALS(outputWS->histogram(3).y()[0], 3)
  }

  void test_merging_detector_scan_workspaces_with_different_positions_throws() {
    auto ws = create_group_detector_scan_workspaces(2);

    auto wsA =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("a1");

    wsA->mutableDetectorInfo().setPosition(std::pair<size_t, size_t>(0, 0),
                                           V3D(5, 6, 7));
    MergeRuns alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: "
                            "matching scan interval but "
                            "positions differ")
  }

  void test_merging_partially_overlapping_detector_scan_workspaces_throws() {
    MatrixWorkspace_sptr a = WorkspaceCreationHelper::
        create2DDetectorScanWorkspaceWithFullInstrument(2, 1000, 2, 0);
    MatrixWorkspace_sptr b = WorkspaceCreationHelper::
        create2DDetectorScanWorkspaceWithFullInstrument(2, 1000, 2, 0, 2);

    AnalysisDataService::Instance().addOrReplace("a", a);
    AnalysisDataService::Instance().addOrReplace("b", b);

    MergeRuns alg;
    alg.initialize();
    alg.setChild(true);
    alg.setPropertyValue("InputWorkspaces", "a, b");
    alg.setPropertyValue("OutputWorkspace", "outWS");
    TS_ASSERT_THROWS_EQUALS(alg.execute(), const std::runtime_error &e,
                            std::string(e.what()),
                            "Cannot merge ComponentInfo: "
                            "scan intervals overlap but not identical")
  }

  void test_merging_detector_scan_workspaces_failure_case() {
    auto ws = create_group_detector_scan_workspaces(2);

    auto wsA =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("a1");
    Property *prop1 = new PropertyWithValue<int>("prop1", 1);
    wsA->mutableRun().addLogData(prop1);

    auto wsB =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("b1");
    Property *prop2 = new PropertyWithValue<int>("prop1", 2);
    wsB->mutableRun().addLogData(prop2);

    MergeRuns alg;
    alg.initialize();
    alg.setPropertyValue("InputWorkspaces", ws->getName());
    alg.setPropertyValue("OutputWorkspace", "outWS");
    alg.setPropertyValue("SampleLogsFail", "prop1");

    TS_ASSERT_THROWS_NOTHING(alg.execute();)

    MatrixWorkspace_sptr outputWS;
    TS_ASSERT_THROWS_NOTHING(
        outputWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "outWS"));

    TS_ASSERT_EQUALS(outputWS->detectorInfo().size(), 2)
    TS_ASSERT_EQUALS(outputWS->detectorInfo().scanCount(), 2)
    TS_ASSERT_EQUALS(outputWS->getNumberHistograms(), 4)

    // Check bins are set correctly
    TS_ASSERT_EQUALS(outputWS->histogram(0).y()[0], 1)
    TS_ASSERT_EQUALS(outputWS->histogram(1).y()[0], 1)
    TS_ASSERT_EQUALS(outputWS->histogram(2).y()[0], 1)
    TS_ASSERT_EQUALS(outputWS->histogram(3).y()[0], 1)
  }

  void test_merging_not_sorted_by_X() {
    // This test checks that issue #22402 has been and remains fixed.
    const BinEdges edges1{{0., 1., 2.}};
    const Counts counts(edges1.size() - 1, 0.);
    const Histogram h1{edges1, counts};
    MatrixWorkspace_sptr ws1 = create<Workspace2D>(1, h1);
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    ws1->mutableRun().addProperty("workspace_number", 1., true);
    const BinEdges edges2{{1., 2., 3.}};
    const Histogram h2{edges2, counts};
    MatrixWorkspace_sptr ws2 = create<Workspace2D>(1, h2);
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    ws2->mutableRun().addProperty("workspace_number", 2., true);
    MergeRuns alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "ws2, ws1"))
    TS_ASSERT_THROWS_NOTHING(
        alg.setProperty("OutputWorkspace", "test_merging_not_sorted_by_X"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_merging_not_sorted_by_X"));
    TS_ASSERT_EQUALS(
        outWS->run().getPropertyValueAsType<double>("workspace_number"), 2.)
    AnalysisDataService::Instance().clear();
  }

  void test_output_independent_from_order_of_inputs() {
    constexpr size_t nWS{5};
    std::array<MatrixWorkspace_sptr, nWS> workspaces;
    const BinEdges edges{{0., 1., 2.}};
    const Counts counts(edges.size() - 1, 1.);
    workspaces[0] = create<Workspace2D>(1, Histogram{edges, counts});
    AnalysisDataService::Instance().addOrReplace("ws1", workspaces[0]);
    workspaces[1] =
        create<Workspace2D>(1, Histogram{edges - 1e-5, 1.01 * counts});
    AnalysisDataService::Instance().addOrReplace("ws2", workspaces[1]);
    workspaces[2] =
        create<Workspace2D>(1, Histogram{edges + 1e-5, 0.99 * counts});
    AnalysisDataService::Instance().addOrReplace("ws3", workspaces[2]);
    workspaces[3] =
        create<Workspace2D>(1, Histogram{edges + 2e-5, 1.02 * counts});
    AnalysisDataService::Instance().addOrReplace("ws4", workspaces[3]);
    workspaces[4] =
        create<Workspace2D>(1, Histogram{edges - 2e-5, 0.98 * counts});
    AnalysisDataService::Instance().addOrReplace("ws5", workspaces[4]);
    std::array<size_t, nWS> indices{{0, 1, 2, 3, 4}};
    MatrixWorkspace_sptr firstWS;
    do {
      std::string inputWS;
      std::string separator;
      for (const auto i : indices) {
        inputWS += separator + workspaces[i]->getName();
        separator = ',';
      }
      MergeRuns alg;
      alg.setRethrows(true);
      alg.initialize();
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspaces", inputWS))
      TS_ASSERT_THROWS_NOTHING(alg.setProperty(
          "OutputWorkspace", "test_output_independent_from_order_of_inputs"))
      TS_ASSERT_THROWS_NOTHING(alg.execute())
      MatrixWorkspace_sptr outWS;
      TS_ASSERT_THROWS_NOTHING(
          outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
              "test_output_independent_from_order_of_inputs"));
      TS_ASSERT(outWS);
      if (firstWS) {
        TS_ASSERT_DELTA(outWS->y(0)[0], firstWS->y(0)[0], 1e-14)
        TS_ASSERT_DELTA(outWS->y(0)[1], firstWS->y(0)[1], 1e-14)
      } else {
        firstWS = outWS;
      }
    } while (std::next_permutation(indices.begin(), indices.end()));
    AnalysisDataService::Instance().clear();
  }

  void test_rebinning_is_done_for_sorted_X() {
    const BinEdges edges{{0., 1., 2.}};
    const Counts counts(edges.size() - 1, 1.);
    MatrixWorkspace_sptr ws1 = create<Workspace2D>(1, Histogram{edges, counts});
    AnalysisDataService::Instance().addOrReplace("ws1", ws1);
    MatrixWorkspace_sptr ws2 =
        create<Workspace2D>(1, Histogram{edges + 10., counts});
    AnalysisDataService::Instance().addOrReplace("ws2", ws2);
    MatrixWorkspace_sptr ws3 =
        create<Workspace2D>(1, Histogram{edges + 5., counts});
    AnalysisDataService::Instance().addOrReplace("ws3", ws3);
    MergeRuns alg;
    alg.setRethrows(true);
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("InputWorkspaces", "ws1,ws2,ws3"))
    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "OutputWorkspace", "test_rebinning_is_done_for_sorted_X"))
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    MatrixWorkspace_sptr outWS;
    TS_ASSERT_THROWS_NOTHING(
        outWS = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            "test_rebinning_is_done_for_sorted_X"));
    TS_ASSERT(outWS);
    const auto &X = outWS->x(0);
    TS_ASSERT_EQUALS(X.size(), 9)
    const std::array<double, 9> expectedX{{0, 1, 2, 5, 6, 7, 10, 11, 12}};
    for (size_t i = 0; i < X.size(); ++i) {
      TS_ASSERT_EQUALS(X[i], expectedX[i])
    }
    AnalysisDataService::Instance().clear();
  }
};

class MergeRunsTestPerformance : public CxxTest::TestSuite {
public:
  static MergeRunsTestPerformance *createSuite() {
    return new MergeRunsTestPerformance();
  }
  static void destroySuite(MergeRunsTestPerformance *suite) { delete suite; }

  MergeRunsTestPerformance() {}

  void setUp() override {
    for (size_t i = 0; i < 10; ++i) {
      // Create a D2B type workspace
      const auto &ws = WorkspaceCreationHelper::
          create2DDetectorScanWorkspaceWithFullInstrument(5000, 1, 25,
                                                          i * 1000);
      const std::string wsName = "a" + std::to_string(i);
      AnalysisDataService::Instance().addOrReplace(wsName, ws);
    }

    m_mergeRuns.initialize();
    m_mergeRuns.setPropertyValue("InputWorkspaces",
                                 "a0, a1, a2, a3, a4, a5, a6, a7, a8, a9");
    m_mergeRuns.setPropertyValue("OutputWorkspace", "outputWS");
  }

  void test_merge_detector_scan_workspaces() { m_mergeRuns.execute(); }

  void tearDown() override {
    for (size_t i = 0; i < 10; ++i) {
      const std::string wsName = "a" + std::to_string(i);
      AnalysisDataService::Instance().remove(wsName);
    }
    AnalysisDataService::Instance().remove("outputWS");
  }

private:
  Mantid::Algorithms::MergeRuns m_mergeRuns;
};

#endif /*MANTID_ALGORITHMS_MERGERUNSTEST_H_*/
