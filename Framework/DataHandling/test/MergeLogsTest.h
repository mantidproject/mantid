#ifndef MANTID_DATAHANDLING_MERGELOGSTEST_H_
#define MANTID_DATAHANDLING_MERGELOGSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/Timer.h"
#include "MantidKernel/System.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/DateAndTime.h"

#include "MantidDataHandling/MergeLogs.h"

using namespace Mantid;
using namespace Mantid::DataHandling;
using namespace Mantid::API;

class MergeLogsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MergeLogsTest *createSuite() { return new MergeLogsTest(); }
  static void destroySuite(MergeLogsTest *suite) { delete suite; }

  void test_Init() {
    // 1. Generate workspace
    API::MatrixWorkspace_sptr mWS =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 100, 100);

    // 2. Init
    Merge2WorkspaceLogs merge;
    merge.initialize();
    TS_ASSERT(merge.isInitialized());
  }

  void test_Merge2() {
    // 1. Generate workspace & 2 logs
    API::MatrixWorkspace_sptr mWS =
        API::WorkspaceFactory::Instance().create("Workspace2D", 1, 100, 100);

    Kernel::TimeSeriesProperty<double> *p1 =
        new Kernel::TimeSeriesProperty<double>("SourceLog1");
    Kernel::TimeSeriesProperty<double> *p2 =
        new Kernel::TimeSeriesProperty<double>("SourceLog2");

    int64_t t1_ns = 1000000;
    int64_t t2_ns = 1000200;
    int64_t dt_ns = 400;
    double v1 = -1.0;
    double v2 = 1.0;
    size_t num1 = 10;
    size_t num2 = 12;

    for (size_t i = 0; i < num1; i++) {
      Kernel::DateAndTime time(t1_ns);
      p1->addValue(time, v1);
      t1_ns += dt_ns;
    }

    for (size_t i = 0; i < num2; i++) {
      Kernel::DateAndTime time(t2_ns);
      p2->addValue(time, v2);
      t2_ns += dt_ns;
    }

    mWS->mutableRun().addProperty(p1);
    mWS->mutableRun().addProperty(p2);

    // 2. Add workspace to data serive
    AnalysisDataService::Instance().addOrReplace("TestDummy", mWS);

    // 3. Running
    Merge2WorkspaceLogs merge;
    merge.initialize();

    TS_ASSERT_THROWS_NOTHING(merge.setPropertyValue("Workspace", "TestDummy"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName1", "SourceLog1"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName2", "SourceLog2"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("MergedLogName", "MergedLog"));

    merge.execute();
    TS_ASSERT(merge.isExecuted());

    // 4. Check result
    API::MatrixWorkspace_sptr mWSout =
        AnalysisDataService::Instance().retrieveWS<API::MatrixWorkspace>(
            "TestDummy");

    Kernel::TimeSeriesProperty<double> *mergprop =
        dynamic_cast<Kernel::TimeSeriesProperty<double> *>(
            mWSout->run().getLogData("MergedLog"));

    TS_ASSERT_EQUALS(mergprop->size(), p1->size() + p2->size());

    std::vector<Kernel::DateAndTime> mergedtimes = mergprop->timesAsVector();
    for (size_t i = 0; i < 2 * num1; i++) {
      Kernel::DateAndTime logtime = mergedtimes[i];
      double logvalue = mergprop->getSingleValue(logtime);
      if (i % 2 == 0) {
        TS_ASSERT_DELTA(logvalue, -1.0, 0.001);
      } else {
        TS_ASSERT_DELTA(logvalue, 1.0, 0.001);
      }
    } // ENDFOR

    for (size_t i = 2 * num1; i < num1 + num2; i++) {
      Kernel::DateAndTime logtime = mergedtimes[i];
      double logvalue = mergprop->getSingleValue(logtime);
      TS_ASSERT_DELTA(logvalue, 1.0, 0.001);
    }
  }
};

#endif /* MANTID_DATAHANDLING_MERGELOGSTEST_H_ */
