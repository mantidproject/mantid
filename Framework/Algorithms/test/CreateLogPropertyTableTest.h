#ifndef MANTID_ALGORITHMS_CREATELOGPROPERTYTABLETEST_H_
#define MANTID_ALGORITHMS_CREATELOGPROPERTYTABLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/TableRow.h"
#include "MantidAPI/Workspace.h"
#include "MantidAlgorithms/CreateLogPropertyTable.h"
#include "MantidDataHandling/Load.h"
#include "MantidDataObjects/EventWorkspace.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <string>
#include <vector>

using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using Mantid::Types::Core::DateAndTime;

class CreateLogPropertyTableTest : public CxxTest::TestSuite {
public:
  void test_init() {
    CreateLogPropertyTable alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  void test_exec() {
    createSampleWorkspace("__CreateLogPropertyTable__A", 12345, 50000000000);
    createSampleWorkspace("__CreateLogPropertyTable__B", 67890, 56000000000);

    CreateLogPropertyTable alg;
    alg.initialize();

    std::string propNamesArray[2] = {"run_number", "run_start"};
    std::vector<std::string> propNames;
    propNames.assign(propNamesArray, propNamesArray + 2);

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspaces",
        "__CreateLogPropertyTable__A, __CreateLogPropertyTable__B"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogPropertyNames", propNames));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "outputTest"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr table =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "outputTest");

    TS_ASSERT(table);

    TableRow row1 = table->getRow(0);
    TableRow row2 = table->getRow(1);

    TS_ASSERT_EQUALS(row1.cell<std::string>(0), "12345");
    TS_ASSERT_EQUALS(row1.cell<std::string>(1), "1990-01-01T00:00:50");
    TS_ASSERT_EQUALS(row2.cell<std::string>(0), "67890");
    TS_ASSERT_EQUALS(row2.cell<std::string>(1), "1990-01-01T00:00:56");
  }

  void test_timeSeries() {
    createSampleWorkspace();

    CreateLogPropertyTable alg;
    alg.initialize();

    std::string propNamesArray[3] = {"run_number", "run_start", "FastSineLog"};
    std::vector<std::string> propNames;
    propNames.assign(propNamesArray, propNamesArray + 3);

    TS_ASSERT_THROWS_NOTHING(alg.setProperty(
        "InputWorkspaces", "__CreateLogPropertyTable__TestWorkspace"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogPropertyNames", propNames));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("OutputWorkspace", "outputTest"));
    TS_ASSERT_THROWS_NOTHING(
        alg.setPropertyValue("TimeSeriesStatistic", "Minimum"));

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(alg.isExecuted());

    ITableWorkspace_sptr table =
        AnalysisDataService::Instance().retrieveWS<ITableWorkspace>(
            "outputTest");

    TS_ASSERT(table);

    TableRow row1 = table->getRow(0);

    TS_ASSERT_EQUALS(row1.cell<std::string>(0), "12345");
    TS_ASSERT_EQUALS(row1.cell<std::string>(1), "1990-01-01T00:00:03");
    TS_ASSERT_EQUALS(row1.cell<std::string>(2), "-1");
  }

private:
  void createSampleWorkspace(
      std::string wsName = "__CreateLogPropertyTable__TestWorkspace",
      int runNumber = 12345, int64_t runStart = 3000000000) {
    using namespace WorkspaceCreationHelper;

    MatrixWorkspace_sptr eventws =
        WorkspaceCreationHelper::create2DWorkspace(1, 1);

    int64_t runstoptime_ns = runStart + 1000000;
    int64_t pulsetime_ns(100000);

    // Run number
    eventws->mutableRun().addProperty("run_number", runNumber);

    // Run start log
    DateAndTime runstarttime(runStart);
    eventws->mutableRun().addProperty("run_start",
                                      runstarttime.toISO8601String());

    // Sine log
    TimeSeriesProperty<double> *sinlog =
        new TimeSeriesProperty<double>("FastSineLog");
    double period = static_cast<double>(pulsetime_ns);
    int64_t curtime_ns = runStart;
    while (curtime_ns < runstoptime_ns) {
      DateAndTime curtime(curtime_ns);
      double value =
          sin(M_PI * static_cast<double>(curtime_ns) / period * 0.25);
      sinlog->addValue(curtime, value);
      curtime_ns += pulsetime_ns / 4;
    }
    eventws->mutableRun().addProperty(sinlog, true);

    AnalysisDataService::Instance().addOrReplace(wsName, eventws);
  }
};

#endif /* MANTID_ALGORITHMS_CREATELOGPROPERTYTABLETEST_H_ */
