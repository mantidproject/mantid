#ifndef ADDSAMPLELOGTEST_H_
#define ADDSAMPLELOGTEST_H_

#include <cxxtest/TestSuite.h>

#include <string>

#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/AddSampleLog.h"
#include "MantidKernel/TimeSeriesProperty.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;

class AddSampleLogTest : public CxxTest::TestSuite {
public:
  void test_Workspace2D() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0);
  }

  void test_EventWorkspace() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::CreateEventWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0);
  }

  void test_CanOverwrite() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0);
    ExecuteAlgorithm(ws, "My Name", "String", "My New Value", 0.0);
  }

  void test_Number() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "Number", "1.234", 1.234);
    ExecuteAlgorithm(ws, "My Name", "Number", "2.456", 2.456);

    ExecuteAlgorithm(ws, "My Name", "Number", "-987654321", -987654321);
    ExecuteAlgorithm(ws, "My Name", "Number", "963", 963);
  }

  void test_BadNumber() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "Number", "OneTwoThreeFour", 0.0, true);
  }

  void test_BadNumberSeries() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ExecuteAlgorithm(ws, "My Name", "Number Series", "FiveSixSeven", 0.0, true);
  }

  void test_NumberSeries() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ws->mutableRun().setStartAndEndTime(DateAndTime("2013-12-18T13:40:00"),
                                        DateAndTime("2013-12-18T13:42:00"));
    ExecuteAlgorithm(ws, "My Name", "Number Series", "1.234", 1.234);
    ExecuteAlgorithm(ws, "My Name", "Number Series", "2.456", 2.456);

    ExecuteAlgorithm(ws, "My Name", "Number Series", "-1", -1);
    ExecuteAlgorithm(ws, "Another Name", "Number Series", "0", 0);
    ExecuteAlgorithm(ws, "Another Name", "Number Series", "123456789",
                     123456789);
  }

  void test_Units() {
    MatrixWorkspace_sptr ws =
        WorkspaceCreationHelper::Create2DWorkspace(10, 10);
    ws->mutableRun().setStartAndEndTime(DateAndTime("2013-12-18T13:40:00"),
                                        DateAndTime("2013-12-18T13:42:00"));
    ExecuteAlgorithm(ws, "My Name", "Number Series", "1.234", 1.234, false,
                     "myUnit");
    ExecuteAlgorithm(ws, "My New Name", "Number", "963", 963, false,
                     "differentUnit");
    ExecuteAlgorithm(ws, "My Name", "String", "My Value", 0.0, false,
                     "stringUnit");
  }

  template <typename T>
  void ExecuteAlgorithm(MatrixWorkspace_sptr testWS, std::string LogName,
                        std::string LogType, std::string LogText,
                        T expectedValue, bool fails = false,
                        std::string LogUnit = "") {
    // add the workspace to the ADS
    AnalysisDataService::Instance().addOrReplace("AddSampleLogTest_Temporary",
                                                 testWS);

    // execute algorithm
    AddSampleLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized())

    alg.setPropertyValue("Workspace", "AddSampleLogTest_Temporary");
    alg.setPropertyValue("LogName", LogName);
    alg.setPropertyValue("LogText", LogText);
    alg.setPropertyValue("LogUnit", LogUnit);
    alg.setPropertyValue("LogType", LogType);
    TS_ASSERT_THROWS_NOTHING(alg.execute())
    if (fails) {
      TS_ASSERT(!alg.isExecuted())
      return;
    } else {
      TS_ASSERT(alg.isExecuted())
    }

    // check output
    MatrixWorkspace_sptr output =
        AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(
            alg.getProperty("Workspace"));

    const Run &wSpaceRun = output->run();
    Property *prop = NULL;
    TS_ASSERT_THROWS_NOTHING(prop = wSpaceRun.getLogData(LogName);)
    if (!prop)
      return;

    if (LogType == "String") {
      TS_ASSERT_EQUALS(prop->value(), LogText);
    } else if (LogType == "Number") {
      auto testProp = dynamic_cast<PropertyWithValue<T> *>(prop);
      TS_ASSERT(testProp);
      TS_ASSERT_DELTA((*testProp)(), expectedValue, 1e-5);
    } else if (LogType == "Number Series") {
      auto testProp = dynamic_cast<TimeSeriesProperty<T> *>(prop);
      TS_ASSERT(testProp);
      TS_ASSERT_EQUALS(testProp->firstTime(),
                       DateAndTime("2013-12-18T13:40:00"));
      TS_ASSERT_DELTA(testProp->firstValue(), expectedValue, 1e-5);
    }
    // cleanup
    AnalysisDataService::Instance().remove(output->getName());
  }
};

#endif /*ADDSAMPLELOGTEST_H_*/
