// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include <string>

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidDataHandling/LoadEmptyInstrument.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using Mantid::Types::Core::DateAndTime;

class AddSampleLogTest : public CxxTest::TestSuite {
private:
  void setStartEndTime(MatrixWorkspace_sptr &ws) {
    ws->mutableRun().setStartAndEndTime(DateAndTime("2013-12-18T13:40:00"), DateAndTime("2013-12-18T13:42:00"));
  }

public:
  void test_Workspace2D() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0);
  }

  void test_EventWorkspace() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::createEventWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0);
  }

  void test_CanOverwrite() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0);
    ExecuteAlgorithm(ws, "My Name", "String", "My New Value", 0.0);
  }

  void test_Number() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name N1", "Number", "1.234", 1.234);
    ExecuteAlgorithm(ws, "My Name N2", "Number", "2.456", 2.456);

    ExecuteAlgorithm(ws, "My Name N3", "Number", "-987654321", -987654321);
    ExecuteAlgorithm(ws, "My Name N4", "Number", "963", 963);
  }

  void test_BadNumber() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name BN", "Number", "OneTwoThreeFour", 0.0, true);
  }

  void test_BadNumberSeries() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "Number Series", "FiveSixSeven", 0.0, true);
  }

  void test_NumberSeries() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    this->setStartEndTime(ws);
    ExecuteAlgorithm(ws, "My Name NS1", "Number Series", "1.234", 1.234);
    ExecuteAlgorithm(ws, "My Name NS1", "Number Series", "2.456", 2.456);
    // Only double is allowed if using default type
    ExecuteAlgorithm(ws, "My Name NS1", "Number Series", "-1", -1);
    ExecuteAlgorithm(ws, "Another Name NS1", "Number Series", "0", 0);
    ExecuteAlgorithm(ws, "Another Name NS2", "Number Series", "123456789", 123456789);
  }

  void test_Units() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    this->setStartEndTime(ws);
    ExecuteAlgorithm(ws, "My Name", "Number Series", "1.234", 1.234, false, "myUnit");
    ExecuteAlgorithm(ws, "My New Name", "Number", "963", 963, false, "differentUnit");
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0, false, "stringUnit");
  }

  void test_number_type() {
    MatrixWorkspace_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    this->setStartEndTime(ws);
    ExecuteAlgorithm(ws, "My Name", "Number Series", "1.234", 1.234, false, "myUnit", "Double");
    ExecuteAlgorithm(ws, "My New Name", "Number", "963", 963, false, "differentUnit", "Int");
    // Can force '963' to be interpreted as a double
    ExecuteAlgorithm(ws, "My New Name", "Number", "963", 963.0, false, "differentUnit", "Double");
    // Should throw error as NumberType defined for a String
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0, true, "stringUnit", "Double", true);
    // Should throw error trying to interpret '1.234' as Int
    ExecuteAlgorithm(ws, "My Name", "Number Series", "1.234", 1.234, true, "myUnit", "Int", true);
  }

  /** Test to add a sample log with values specified by a MatrixWorkspace
   * @brief test_matrix_workspace
   */
  void test_matrix_workspace() {
    // create the workspace to add sample log to
    MatrixWorkspace_sptr target_ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);

    // create workspace with time series property's value. which has 2 spectra
    // and 10 values
    MatrixWorkspace_sptr ts_ws = WorkspaceCreationHelper::create2DWorkspace(2, 10);

    for (size_t i = 0; i < 10; ++i) {
      // for X
      for (size_t ws_index = 0; ws_index < 2; ++ws_index)
        ts_ws->mutableX(ws_index)[i] = static_cast<double>(i) * 0.1;
      // for Y
      ts_ws->mutableY(1)[i] = 3. * static_cast<double>(i * i) + 0.5;
    }

    // add the workspace to the ADS
    AnalysisDataService::Instance().addOrReplace("AddSampleLogTest_Temporary", target_ws);
    AnalysisDataService::Instance().addOrReplace("TimeSeries", ts_ws);

    // execute algorithm
    AddSampleLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())

    alg.setPropertyValue("Workspace", "AddSampleLogTest_Temporary");
    alg.setPropertyValue("LogName", "NewLog");
    alg.setPropertyValue("LogUnit", "Degree");
    alg.setPropertyValue("LogType", "Number Series");
    alg.setPropertyValue("NumberType", "Double");
    alg.setPropertyValue("TimeSeriesWorkspace", "TimeSeries");
    alg.setProperty("WorkspaceIndex", 1);
    alg.setProperty("TimeUnit", "Second");
    alg.setProperty("RelativeTime", true);

    // execute
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    TS_ASSERT(alg.isExecuted())

    // check result
    TS_ASSERT(target_ws->run().hasProperty("NewLog"));
    TimeSeriesProperty<double> *newlog =
        dynamic_cast<TimeSeriesProperty<double> *>(target_ws->run().getProperty("NewLog"));
    TS_ASSERT(newlog);
    TS_ASSERT_EQUALS(newlog->size(), 10);
    TS_ASSERT_DELTA(newlog->nthValue(1), 3.5, 0.0001);

    int64_t t0ns = newlog->nthTime(0).totalNanoseconds();
    int64_t t1ns = newlog->nthTime(1).totalNanoseconds();
    TS_ASSERT_EQUALS(t1ns - t0ns, static_cast<int64_t>(1.E8));
  }

  template <typename T>
  void ExecuteAlgorithm(const MatrixWorkspace_sptr &testWS, const std::string &LogName, const std::string &LogType,
                        const std::string &LogText, T expectedValue, bool fails = false,
                        const std::string &LogUnit = "", const std::string &NumberType = "AutoDetect",
                        bool throws = false, bool updateInstrumentParams = false) {
    // add the workspace to the ADS
    AnalysisDataService::Instance().addOrReplace("AddSampleLogTest_Temporary", testWS);

    // execute algorithm
    AddSampleLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    if (throws)
      alg.setRethrows(true);
    TS_ASSERT(alg.isInitialized())

    alg.setPropertyValue("Workspace", "AddSampleLogTest_Temporary");
    alg.setPropertyValue("LogName", LogName);
    alg.setPropertyValue("LogText", LogText);
    alg.setPropertyValue("LogUnit", LogUnit);
    alg.setPropertyValue("LogType", LogType);
    alg.setPropertyValue("NumberType", NumberType);
    alg.setProperty("UpdateInstrumentParameters", updateInstrumentParams);
    if (throws) {
      TS_ASSERT_THROWS_ANYTHING(alg.execute())
      return;
    }
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    if (fails) {
      TS_ASSERT(!alg.isExecuted())
      return;
    } else {
      TS_ASSERT(alg.isExecuted())
    }

    // check output
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(alg.getProperty("Workspace"));

    const Run &wSpaceRun = output->run();
    Property *prop = nullptr;
    TS_ASSERT_THROWS_NOTHING(prop = wSpaceRun.getLogData(LogName);)
    if (!prop)
      return;

    std::cout << "Log type: " << LogType << "\n";

    if (LogType == "String") {
      TS_ASSERT_EQUALS(prop->value(), LogText);
    } else if (LogType == "Number") {
      auto testProp = dynamic_cast<PropertyWithValue<T> *>(prop);
      TS_ASSERT(testProp);
      TS_ASSERT_DELTA((*testProp)(), expectedValue, 1e-5);
    } else if (LogType == "Number Series") {
      auto testProp = dynamic_cast<TimeSeriesProperty<T> *>(prop);
      TS_ASSERT(testProp);
      TS_ASSERT_EQUALS(testProp->firstTime(), DateAndTime("2013-12-18T13:40:00"));
      TS_ASSERT_DELTA(testProp->firstValue(), expectedValue, 1e-5);
    }
    // cleanup
    AnalysisDataService::Instance().remove(output->getName());
  }

  /*
   * SNAP has detector positions that depend on the sample logs. This test load the instrument add the logs, then
   * verifies the derived positions are set correctly.
   */
  void test_instrumentWithParameters() {
    const std::string wkspName("SNAP_det_pos");
    const Mantid::Kernel::V3D ORIGIN(0, 0, 0);

    // load the empty instrument
    Mantid::DataHandling::LoadEmptyInstrument loadAlg;
    loadAlg.initialize();
    loadAlg.setProperty("InstrumentName", "SNAP");
    loadAlg.setProperty("OutputWorkspace", wkspName);
    loadAlg.execute();
    TS_ASSERT(loadAlg.isExecuted());
    // get the workspace
    MatrixWorkspace_sptr ws =
        std::dynamic_pointer_cast<MatrixWorkspace>(AnalysisDataService::Instance().retrieve(wkspName));
    this->setStartEndTime(ws);

    // values taken from logs of SNAP_57514
    constexpr double det_lin1{0.045};
    constexpr double det_lin2{0.043};
    constexpr double det_arc1{-65.3};
    constexpr double det_arc2{104.95};

    // add the logs for detector information and make sure the logs are set correctly
    // none of these update the instrument
    ExecuteAlgorithm(ws, "det_lin1", "Number Series", "0.045", det_lin1);
    ExecuteAlgorithm(ws, "det_lin2", "Number Series", "0.043", det_lin2);
    ExecuteAlgorithm(ws, "det_arc1", "Number Series", "-65.3", det_arc1);
    ExecuteAlgorithm(ws, "det_arc2", "Number Series", "104.95", det_arc2);

    // not updating the instrument puts the banks at the origin
    const auto westPosBad = ws->getInstrument()->getComponentByName("West")->getPos();
    const auto eastPosBad = ws->getInstrument()->getComponentByName("East")->getPos();
    TS_ASSERT_EQUALS(westPosBad, ORIGIN);
    TS_ASSERT_EQUALS(eastPosBad, ORIGIN);

    // re-run the last call and requests for the instrument to be updated, the other function parameters are the default
    // values
    ExecuteAlgorithm(ws, "det_arc2", "Number Series", "104.95", det_arc2, false, "", "AutoDetect", false, true);

    // positions for the center of SNAP's two sides
    const auto westPosGood = ws->getInstrument()->getComponentByName("West")->getPos();
    const auto eastPosGood = ws->getInstrument()->getComponentByName("East")->getPos();

    // check the center angles from downstream
    // V3D::angle returns radians
    const Mantid::Kernel::V3D downstream(0, 0, 1);
    TS_ASSERT_EQUALS(westPosGood.angle(downstream) * 180 / M_PI, abs(det_arc1));
    TS_ASSERT_EQUALS(eastPosGood.angle(downstream) * 180 / M_PI, abs(det_arc2));

    // check the center distance - detector is 0.5m + det_lin motor
    TS_ASSERT_EQUALS(westPosGood.distance(ORIGIN), 0.5 + det_lin1);
    TS_ASSERT_EQUALS(eastPosGood.distance(ORIGIN), 0.5 + det_lin2);
  }
};
