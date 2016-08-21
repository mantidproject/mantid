#ifndef MERGERUNSTEST_H_
#define MERGERUNSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <stdarg.h>

#include "MantidAPI/WorkspaceGroup.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/MergeRuns.h"
#include "MantidAlgorithms/GroupWorkspaces.h"
#include "MantidDataHandling/LoadEventPreNexus.h"
#include "MantidGeometry/Instrument.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;
using namespace Mantid::Kernel;
using Mantid::DataHandling::LoadEventPreNexus;

class MergeRunsTest : public CxxTest::TestSuite {

private:
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
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

  template<typename T>
  WorkspaceGroup_sptr create_workspace_with_sample_logs(std::string merge_type, T value_1, T value_2) {
    MatrixWorkspace_sptr a =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            2, 1000, true);
    MatrixWorkspace_sptr b =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(
            2, 1000, true);

    Property *prop1 = new PropertyWithValue<T>("prop", value_1);
    Property *prop2 = new PropertyWithValue<T>("prop", value_2);

    a->mutableRun().addLogData(prop1);
    b->mutableRun().addLogData(prop2);

    a->instrumentParameters().addString(a->getInstrument()->getComponentID(), merge_type, "prop");
    b->instrumentParameters().addString(b->getInstrument()->getComponentID(), merge_type, "prop");

    WorkspaceGroup_sptr group = boost::make_shared<WorkspaceGroup>();
    group->addWorkspace(a);
    group->addWorkspace(b);

    AnalysisDataService::Instance().addOrReplace("a1", a);
    AnalysisDataService::Instance().addOrReplace("b1", b);
    AnalysisDataService::Instance().addOrReplace("group1", group);
    return group;
  }

  void do_test_treat_as_non_period_groups(WorkspaceGroup_sptr input) {
    MatrixWorkspace_sptr sampleInputWorkspace =
        boost::dynamic_pointer_cast<MatrixWorkspace>(input->getItem(0));
    const double uniformSignal = sampleInputWorkspace->readY(0)[0];
    const double uniformError = sampleInputWorkspace->readE(0)[0];
    const size_t nXValues = sampleInputWorkspace->readX(0).size();

    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspaces", input->name() + "," + input->name()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "out"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    MatrixWorkspace_sptr wsOut = Mantid::API::AnalysisDataService::Instance()
                                     .retrieveWS<MatrixWorkspace>("out");
    TS_ASSERT(wsOut != NULL);
    for (size_t j = 0; j < wsOut->getNumberHistograms(); ++j) {
      using Mantid::MantidVec;
      MantidVec xValues = wsOut->readX(j);
      MantidVec yValues = wsOut->readY(j);
      MantidVec eValues = wsOut->readE(j);
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
        "in1", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 1));
    AnalysisDataService::Instance().add(
        "in2", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 1));
    AnalysisDataService::Instance().add(
        "in3", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 1));
    AnalysisDataService::Instance().add(
        "in4", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 5, 20));
    AnalysisDataService::Instance().add(
        "in5", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 5, 3.5, 2));
    AnalysisDataService::Instance().add(
        "in6", WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 3, 2, 2));
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
        output->mutableRun().getProperty("proton_charge"));
    log1 = log->realSize();
    nev1 = output->getNumberEvents();
    pc1 = output->mutableRun().getProtonCharge();

    TS_ASSERT_THROWS_NOTHING(
        output = AnalysisDataService::Instance().retrieveWS<EventWorkspace>(
            "cncs2");)
    log = dynamic_cast<TimeSeriesProperty<double> *>(
        output->mutableRun().getProperty("proton_charge"));
    log2 = log->realSize();
    nev2 = output->getNumberEvents();
    pc2 = output->mutableRun().getProtonCharge();

    TS_ASSERT_THROWS_NOTHING(
        output =
            AnalysisDataService::Instance().retrieveWS<EventWorkspace>(wsname));
    TS_ASSERT(output);

    // This many pixels total at CNCS
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 51200);

    log = dynamic_cast<TimeSeriesProperty<double> *>(
        output->mutableRun().getProperty("proton_charge"));
    logTot = log->realSize();
    nevTot = output->getNumberEvents();
    pcTot = output->mutableRun().getProtonCharge();

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
        WorkspaceCreationHelper::CreateEventWorkspace(3, 10, 100, 0.0, 1.0, 3);
    AnalysisDataService::Instance().addOrReplace(
        "ev1", boost::dynamic_pointer_cast<MatrixWorkspace>(ev1)); // 100 ev
    AnalysisDataService::Instance().addOrReplace(
        "ev2", boost::dynamic_pointer_cast<MatrixWorkspace>(
                   WorkspaceCreationHelper::CreateEventWorkspace(
                       3, 10, 100, 0.0, 1.0, 2))); // 200 ev
    AnalysisDataService::Instance().addOrReplace(
        "ev3", boost::dynamic_pointer_cast<MatrixWorkspace>(
                   WorkspaceCreationHelper::CreateEventWorkspace(
                       3, 10, 100, 0.0, 1.0, 2, 100))); // 200 events per
                                                        // spectrum, but the
                                                        // spectra are at
                                                        // different pixel ids
    // Make one with weird units
    MatrixWorkspace_sptr ev4 = boost::dynamic_pointer_cast<MatrixWorkspace>(
        WorkspaceCreationHelper::CreateEventWorkspace(3, 10, 100, 0.0, 1.0, 2,
                                                      100));
    ev4->setYUnit("Microfurlongs per Megafortnights");
    AnalysisDataService::Instance().addOrReplace("ev4_weird_units", ev4);
    AnalysisDataService::Instance().addOrReplace(
        "ev5", boost::dynamic_pointer_cast<MatrixWorkspace>(
                   WorkspaceCreationHelper::CreateEventWorkspace(
                       5, 10, 100, 0.0, 1.0, 2, 100))); // 200 events per
                                                        // spectrum, but the
                                                        // spectra are at
                                                        // different pixel ids
    ev6 = WorkspaceCreationHelper::CreateEventWorkspace(6, 10, 100, 0.0, 1.0,
                                                        3); // ids 0-5
    AnalysisDataService::Instance().addOrReplace(
        "ev6", boost::dynamic_pointer_cast<MatrixWorkspace>(ev6));
    // a 2d workspace with the value 2 in each bin
    AnalysisDataService::Instance().addOrReplace(
        "in2D",
        WorkspaceCreationHelper::Create2DWorkspaceBinned(3, 10, 0.0, 1.0));

    std::vector<std::vector<int>> groups;

    groups.clear();
    groups.push_back(makeVector(3, 0, 1, 2));
    groups.push_back(makeVector(3, 3, 4, 5));
    evg1 = WorkspaceCreationHelper::CreateGroupedEventWorkspace(groups, 100);
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
    evg2 = WorkspaceCreationHelper::CreateGroupedEventWorkspace(groups, 100);
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
      const auto &outX = output->readX(i);
      const auto &outY = output->readY(i);
      const auto &outE = output->readE(i);
      for (size_t j = 0; j < xsize; ++j) {
        TS_ASSERT_DELTA(outX[j], input->readX(i)[j], 1e-12);
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
    TS_ASSERT(output->getSpectrum(3)
                  .hasDetectorID(5)); // Leftover from the ev1 workspace

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
        WorkspaceCreationHelper::Create2DWorkspace123(3, 10, 1);
    badIn->dataX(0) = std::vector<double>(11, 2.0);
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

    const Mantid::MantidVec &X = output->readX(0);
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

    const Mantid::MantidVec &X = output->readX(0);
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

    const Mantid::MantidVec &X = output->readX(0);
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
    alg.setPropertyValue("InputWorkspaces", a->name() + "," + b->name());
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
    const double uniformSignal = sampleNestedInputWorkspace->readY(0)[0];
    const double uniformError = sampleNestedInputWorkspace->readE(0)[0];
    const size_t nXValues = sampleNestedInputWorkspace->readX(0).size();

    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "InputWorkspaces", input->name() + "," + input->name()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outer"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    WorkspaceGroup_sptr wsgroup =
        Mantid::API::AnalysisDataService::Instance().retrieveWS<WorkspaceGroup>(
            "outer");
    TS_ASSERT(wsgroup != NULL);
    TS_ASSERT_EQUALS(input->size(), wsgroup->size());
    // Loop through each workspace in the group
    for (size_t i = 0; i < wsgroup->size(); ++i) {
      MatrixWorkspace_sptr ws =
          boost::dynamic_pointer_cast<MatrixWorkspace>(wsgroup->getItem(i));
      TS_ASSERT(ws != NULL);
      TS_ASSERT_EQUALS(expectedNumHistograms, ws->getNumberHistograms());
      // Loop through each histogram in each workspace
      for (size_t j = 0; j < ws->getNumberHistograms(); ++j) {
        using Mantid::MantidVec;
        MantidVec xValues = ws->readX(j);
        MantidVec yValues = ws->readY(j);
        MantidVec eValues = ws->readE(j);
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

  void do_test_mergeSampleLogs(WorkspaceGroup_sptr input, std::string mergeType, std::string result, int filesMerged, bool noOutput = false) {
    MatrixWorkspace_sptr input0 = boost::dynamic_pointer_cast<MatrixWorkspace>(input->getItem(0));
    MatrixWorkspace_sptr input1 = boost::dynamic_pointer_cast<MatrixWorkspace>(input->getItem(1));

    MergeRuns alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("InputWorkspaces", input0->name() + "," + input1->name()));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("OutputWorkspace", "outWS"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    MatrixWorkspace_sptr output;
    if (noOutput) {
        TS_ASSERT_THROWS(AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"), Mantid::Kernel::Exception::NotFoundError);
        return;
    } else {
        TS_ASSERT_THROWS_NOTHING(output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>("outWS"));
    }

    Property *prop;

    TS_ASSERT_EQUALS(output->getSpectrum(0).dataY().front(), 2.0 * filesMerged);

    if (mergeType.compare("sample_logs_list") == 0) {
      prop = output->mutableRun().getLogData("prop_list");
      TS_ASSERT_EQUALS(prop->value(), result);
    } else {
      prop = output->mutableRun().getLogData("prop");
      TS_ASSERT_EQUALS(prop->value(), result);
    }

    sample_logs_teardown();
  }

  void sample_logs_teardown() {
    AnalysisDataService::Instance().remove("a1");
    AnalysisDataService::Instance().remove("b1");
    AnalysisDataService::Instance().remove("group1");
    AnalysisDataService::Instance().remove("outWS");
  }

  void test_mergeSampleLogs_average() {
    std::string mergeType = "sample_logs_average";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "1.5", 2);
  }

  void test_mergeSampleLogs_min() {
    std::string mergeType = "sample_logs_min";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "1", 2);
  }

  void test_mergeSampleLogs_max() {
    std::string mergeType = "sample_logs_max";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "2", 2);
  }

  void test_mergeSampleLogs_sum() {
    std::string mergeType = "sample_logs_sum";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "3", 2);
  }

  void test_mergeSampleLogs_list() {
    std::string mergeType = "sample_logs_list";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "1, 2", 2);
  }

  void test_mergeSampleLogs_warn() {
    std::string mergeType = "sample_logs_warn";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "1", 2);
  }

  void test_mergeSampleLogs_fail() {
    std::string mergeType = "sample_logs_fail";
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0), mergeType, "1", 1);
    do_test_mergeSampleLogs(create_workspace_with_sample_logs<double>(mergeType, 1.0, 1.0), mergeType, "1", 2);
  }

  void test_mergeSampleLogs_non_existent_log_is_ignored() {
    std::string mergeType = "sample_logs_average";
    WorkspaceGroup_sptr gws = create_workspace_with_sample_logs<double>(mergeType, 1.0, 2.0);
    MatrixWorkspace_sptr a = boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    a->instrumentParameters().addString(a->getInstrument()->getComponentID(), mergeType, "prop, non_existent");
    do_test_mergeSampleLogs(gws, mergeType, "1.5", 2);
  }

  void test_mergeSampleLogs_log_used_twice_throws_error() {
    std::string mergeTypeAverage = "sample_logs_average";
    std::string mergeTypeSum = "sample_logs_sum";
    WorkspaceGroup_sptr gws = create_workspace_with_sample_logs<double>(mergeTypeAverage, 1.0, 2.0);
    MatrixWorkspace_sptr a = boost::dynamic_pointer_cast<MatrixWorkspace>(gws->getItem(0));
    a->instrumentParameters().addString(a->getInstrument()->getComponentID(), mergeTypeSum, "prop");

    do_test_mergeSampleLogs(gws, mergeTypeAverage, "1.5", 1, true);
  }

private:
  MergeRuns merge;
};

#endif /*MERGERUNSTEST_H_*/
