// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAlgorithms/SumEventsByLogValue.h"
#include "MantidDataHandling/Load.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/TimeSeriesProperty.h"

using Mantid::Algorithms::SumEventsByLogValue;
using Mantid::DataObjects::EventWorkspace_sptr;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

class SumEventsByLogValueTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SumEventsByLogValueTest *createSuite() { return new SumEventsByLogValueTest(); }
  static void destroySuite(SumEventsByLogValueTest *suite) { delete suite; }

  void test_validators() {
    SumEventsByLogValue alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());

    // InputWorkspace has to be an EventWorkspace
    TS_ASSERT_THROWS(alg.setProperty("InputWorkspace", WorkspaceCreationHelper::create2DWorkspace(1, 1)),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", WorkspaceCreationHelper::createEventWorkspace()));

    // LogName must not be empty
    TS_ASSERT_THROWS(alg.setProperty("LogName", ""), const std::invalid_argument &);
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
    // Add an EMPTY time-series property
    auto emptyTSP = new TimeSeriesProperty<int>("EmptyTSP");
    ws->mutableRun().addLogData(emptyTSP);

    SumEventsByLogValue alg;
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

    // Check protest when empty tsp log given
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "emptyTSP"));
    errorMap = alg.validateInputs();
    TS_ASSERT_EQUALS(errorMap.size(), 1);
    TS_ASSERT_EQUALS(errorMap.begin()->first, "LogName");

    // Check it's happy when a non-empty tsp is given
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "TSP"));
    errorMap = alg.validateInputs();
    TS_ASSERT(errorMap.empty());
  }

  void test_text_property() {
    auto alg = setupAlg("textProp");
    TS_ASSERT(!alg->execute());
  }

  void test_double_property_fails_if_no_rebin_parameters() {
    auto alg = setupAlg("doubleProp");
    TS_ASSERT(!alg->execute());
  }

  void test_double_property() {
    auto alg = setupAlg("doubleProp");
    alg->setChild(true);
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputBinning", "2.5,1,3.5"));
    TS_ASSERT(alg->execute());

    Workspace_const_sptr out = alg->getProperty("OutputWorkspace");
    MatrixWorkspace_const_sptr outWS = std::dynamic_pointer_cast<const MatrixWorkspace>(out);
    TS_ASSERT_EQUALS(outWS->getNumberHistograms(), 1);
    TS_ASSERT_EQUALS(outWS->y(0)[0], 300);
  }

  void test_double_property_with_number_of_bins_only() {
    auto alg = setupAlg("doubleProp");
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputBinning", "3"));
    TS_ASSERT(alg->execute());
  }

  void test_integer_property() {
    auto alg = setupAlg("integerProp");
    alg->setChild(true);
    TS_ASSERT(alg->execute());

    Workspace_sptr out = alg->getProperty("OutputWorkspace");
    auto outWS = std::dynamic_pointer_cast<ITableWorkspace>(out);
    TS_ASSERT_EQUALS(outWS->rowCount(), 2);
    // TS_ASSERT_EQUALS( outWS->columnCount(), 4 );
    TS_ASSERT_EQUALS(outWS->Int(0, 0), 1);
    TS_ASSERT_EQUALS(outWS->Int(0, 1), 270);
    TS_ASSERT_EQUALS(outWS->Double(0, 2), std::sqrt(270.0));
    TS_ASSERT_EQUALS(outWS->Int(1, 0), 2);
    TS_ASSERT_EQUALS(outWS->Int(1, 1), 30);
    TS_ASSERT_EQUALS(outWS->Double(1, 2), std::sqrt(30.0));

    // Check the times & the proton charge
    TS_ASSERT_EQUALS(outWS->Double(0, 3), 89.0);
    TS_ASSERT_EQUALS(outWS->Double(1, 3), 10.0);
    TS_ASSERT_EQUALS(outWS->Double(0, 4), 89.0E7);
    TS_ASSERT_EQUALS(outWS->Double(1, 4), 10.0E7);
    // Save more complex tests for a system test
  }

  void test_loadNexus() {
    EventWorkspace_sptr WS;
    auto loader = AlgorithmManager::Instance().create("LoadEventNexus");
    loader->initialize();
    loader->setPropertyValue("Filename", "HYSA_2934.nxs.h5");
    loader->setPropertyValue("LoadMonitors", "1");
    loader->setPropertyValue("OutputWorkspace", "testInput");
    loader->execute();
    TS_ASSERT(loader->isExecuted());

    WS = AnalysisDataService::Instance().retrieveWS<Mantid::DataObjects::EventWorkspace>("testInput");
    TS_ASSERT(WS); // workspace is loaded
    auto monWS = std::dynamic_pointer_cast<Mantid::DataObjects::EventWorkspace>(WS->monitorWorkspace());
    IAlgorithm_sptr alg = std::make_shared<SumEventsByLogValue>();
    alg->initialize();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", WS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", monWS));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("LogName", "scan_index"));
    alg->setChild(true);
    TS_ASSERT(alg->execute());

    Workspace_sptr out = alg->getProperty("OutputWorkspace");
    auto outWS = std::dynamic_pointer_cast<ITableWorkspace>(out);
    TS_ASSERT_EQUALS(outWS->rowCount(), 15);
    TS_ASSERT_EQUALS(outWS->Double(0, 3), 52.53912528699999);
    TS_ASSERT_EQUALS(outWS->Double(0, 4), 47161646710.0);
    TS_ASSERT_EQUALS(outWS->Double(1, 3), 6.44623244);
    TS_ASSERT_EQUALS(outWS->Double(1, 4), 5793533550.0);
    TS_ASSERT_EQUALS(outWS->Double(2, 3), 6.498204369);
    TS_ASSERT_EQUALS(outWS->Double(2, 4), 5837280140.0);
    TS_ASSERT_EQUALS(outWS->Double(3, 3), 6.684533796);
    TS_ASSERT_EQUALS(outWS->Double(3, 4), 6007716450.0);
    TS_ASSERT_EQUALS(outWS->Double(4, 3), 6.437640537);
    TS_ASSERT_EQUALS(outWS->Double(4, 4), 5784639810.0);
    TS_ASSERT_EQUALS(outWS->Double(5, 3), 6.724165215);
    TS_ASSERT_EQUALS(outWS->Double(5, 4), 6038131490.0);
    TS_ASSERT_EQUALS(outWS->Double(6, 3), 6.691884511);
    TS_ASSERT_EQUALS(outWS->Double(6, 4), 6028468300.0);
    TS_ASSERT_EQUALS(outWS->Double(7, 3), 6.744685377);
    TS_ASSERT_EQUALS(outWS->Double(7, 4), 6076178440.0);
    TS_ASSERT_EQUALS(outWS->Double(8, 3), 6.446148924);
    TS_ASSERT_EQUALS(outWS->Double(8, 4), 5806852510.0);
    TS_ASSERT_EQUALS(outWS->Double(9, 3), 15.521905456);
    TS_ASSERT_EQUALS(outWS->Double(9, 4), 5389996420.0);
    TS_ASSERT_EQUALS(outWS->Double(10, 3), 6.520652694);
    TS_ASSERT_EQUALS(outWS->Double(10, 4), 5870629140.0);
    TS_ASSERT_EQUALS(outWS->Double(11, 3), 6.687109917);
    TS_ASSERT_EQUALS(outWS->Double(11, 4), 6009915210.0);
    TS_ASSERT_EQUALS(outWS->Double(12, 3), 6.732920325);
    TS_ASSERT_EQUALS(outWS->Double(12, 4), 5936084950.0);
    TS_ASSERT_EQUALS(outWS->Double(13, 3), 6.475679524);
    TS_ASSERT_EQUALS(outWS->Double(13, 4), 5805181920.0);
    TS_ASSERT_EQUALS(outWS->Double(14, 3), 3.196203628);
    TS_ASSERT_EQUALS(outWS->Double(14, 4), 2889833200.0);
  }

private:
  IAlgorithm_sptr setupAlg(const std::string &logName) {
    IAlgorithm_sptr alg = std::make_shared<SumEventsByLogValue>();
    alg->initialize();
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("InputWorkspace", createWorkspace()));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("OutputWorkspace", "outws"));
    TS_ASSERT_THROWS_NOTHING(alg->setProperty("LogName", logName));

    return alg;
  }

  EventWorkspace_sptr createWorkspace() {
    EventWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspace(3, 1);
    Run &run = ws->mutableRun();

    DateAndTime run_start("2010-01-01T00:00:00");

    auto dblTSP = new TimeSeriesProperty<double>("doubleProp");
    dblTSP->addValue(run_start, 3.0);
    run.addProperty(dblTSP);

    auto textTSP = new TimeSeriesProperty<std::string>("textProp");
    textTSP->addValue(run_start, "ON");
    run.addProperty(textTSP);

    auto intTSP = new TimeSeriesProperty<int>("integerProp");
    intTSP->addValue(run_start, 1);
    intTSP->addValue(run_start + 10.0, 2);
    intTSP->addValue(run_start + 20.0, 1);
    run.addProperty(intTSP);

    auto proton_charge = new TimeSeriesProperty<double>("proton_charge");
    proton_charge->setUnits("picoCoulombs");
    for (int i = 0; i < 100; ++i) {
      proton_charge->addValue(run_start + static_cast<double>(i), 1.0E7);
    }
    run.addProperty(proton_charge);

    return ws;
  }
};

class SumEventsByLogValueTestPerformance : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SumEventsByLogValueTestPerformance *createSuite() { return new SumEventsByLogValueTestPerformance(); }
  static void destroySuite(SumEventsByLogValueTestPerformance *suite) { delete suite; }

  SumEventsByLogValueTestPerformance() {
    ws = WorkspaceCreationHelper::createEventWorkspace(100, 100, 1000);
    // Add a bunch of logs
    std::vector<DateAndTime> times;
    std::vector<int> index;
    std::vector<double> dbl1, dbl2;
    DateAndTime startTime("2010-01-01T00:00:00");
    for (int i = 0; i < 100; ++i) {
      times.emplace_back(startTime + i * 10.0);
      index.emplace_back(i);
      dbl1.emplace_back(i * 0.1);
      dbl2.emplace_back(6.0);
    }

    auto scan_index = new TimeSeriesProperty<int>("scan_index");
    scan_index->addValues(times, index);
    ws->mutableRun().addProperty(scan_index);
    auto dbl_prop1 = new TimeSeriesProperty<double>("some_prop");
    auto dbl_prop2 = new TimeSeriesProperty<double>("some_other_prop");
    dbl_prop1->addValues(times, dbl1);
    dbl_prop2->addValues(times, dbl2);
    ws->mutableRun().addProperty(dbl_prop1);
    ws->mutableRun().addProperty(dbl_prop2);

    auto proton_charge = new TimeSeriesProperty<double>("proton_charge");
    for (int i = 0; i < 1000; ++i) {
      proton_charge->addValue(startTime + double(i), 1.0e6);
    }
    ws->mutableRun().addProperty(proton_charge);
  }

  void test_table_output() {
    SumEventsByLogValue alg;
    alg.initialize();
    alg.setProperty("InputWorkspace", ws);
    alg.setProperty("OutputWorkspace", "outws");
    alg.setProperty("LogName", "scan_index");
    TS_ASSERT(alg.execute());
  }

private:
  EventWorkspace_sptr ws;
};
