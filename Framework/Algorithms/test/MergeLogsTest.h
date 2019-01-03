// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MERGELOGSTEST_H_
#define MANTID_ALGORITHMS_MERGELOGSTEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/Run.h"
#include "MantidAlgorithms/MergeLogs.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidDataObjects/WorkspaceCreation.h"
#include "MantidHistogramData/Histogram.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include <cxxtest/TestSuite.h>
#include <string.h>
using namespace Mantid::API;
using namespace Mantid::Algorithms;
using namespace Mantid::DataObjects;
using namespace Mantid::HistogramData;
using namespace Mantid::Kernel;
using namespace Mantid::Types::Core;

class MergeLogsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MergeLogsTest *createSuite() { return new MergeLogsTest(); }
  static void destroySuite(MergeLogsTest *suite) { delete suite; }

  void test_init() {
    MergeLogs merge;
    TS_ASSERT_THROWS_NOTHING(merge.initialize());
    TS_ASSERT(merge.isInitialized());
  }

  void test_merge() {
    auto p1 = createLog("SourceLog1", 1000000, 400, 10, -1.0);
    auto p2 = createLog("SourceLog2", 1000200, 400, 15, 3.0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(1, BinEdges(101));
    ws->mutableRun().addProperty(p1);
    ws->mutableRun().addProperty(p2);

    MergeLogs merge;
    merge.initialize();
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("Workspace", ws));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName1", "SourceLog1"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName2", "SourceLog2"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("MergedLogName", "MergedLog"));
    TS_ASSERT_THROWS_NOTHING(merge.execute());
    TS_ASSERT(merge.isExecuted());

    testLogValues(ws, "SourceLog1", 10, -1.0);
    testLogValues(ws, "SourceLog2", 15, 3.0);
    testLogValues(ws, "MergedLog", 10, 15, -1.0, 3.0);
  }

  void test_replace_values_by_defaults() {
    auto p1 = createLog("SourceLog1", 1000000, 400, 10, -1.0);
    auto p2 = createLog("SourceLog2", 1000200, 400, 15, 1.0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(1, BinEdges(101));
    ws->mutableRun().addProperty(p1);
    ws->mutableRun().addProperty(p2);

    MergeLogs merge;
    merge.initialize();
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("Workspace", ws));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName1", "SourceLog1"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName2", "SourceLog2"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("MergedLogName", "MergedLog"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("ResetLogValue", true));
    TS_ASSERT_THROWS_NOTHING(merge.execute());
    TS_ASSERT(merge.isExecuted());

    testLogValues(ws, "SourceLog1", 10, -1.0);
    testLogValues(ws, "SourceLog2", 15, 1.0);
    testLogValues(ws, "MergedLog", 10, 15, 0.0, 1.0);
  }

  void test_replace_values_non_defaults() {
    auto p1 = createLog("SourceLog1", 1000000, 400, 10, -1.0);
    auto p2 = createLog("SourceLog2", 1000200, 400, 15, 1.0);
    MatrixWorkspace_sptr ws = create<Workspace2D>(1, BinEdges(101));
    ws->mutableRun().addProperty(p1);
    ws->mutableRun().addProperty(p2);

    MergeLogs merge;
    merge.initialize();
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("Workspace", ws));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName1", "SourceLog1"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogName2", "SourceLog2"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("MergedLogName", "MergedLog"));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("ResetLogValue", true));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogValue1", 2.2));
    TS_ASSERT_THROWS_NOTHING(merge.setProperty("LogValue2", 3.3));
    TS_ASSERT_THROWS_NOTHING(merge.execute());
    TS_ASSERT(merge.isExecuted());

    testLogValues(ws, "SourceLog1", 10, -1.0);
    testLogValues(ws, "SourceLog2", 15, 1.0);
    testLogValues(ws, "MergedLog", 10, 15, 2.2, 3.3);
  }

  Property *createLog(const std::string &name, int64_t t_ns, int64_t dt_ns,
                      size_t number, double value) {
    TimeSeriesProperty<double> *log = new TimeSeriesProperty<double>(name);
    for (size_t i = 0; i < number; ++i) {
      DateAndTime time(t_ns);
      log->addValue(time, value);
      t_ns += dt_ns;
    }
    log->setUnits("TimeOfFligths");
    return log;
  }

  void testLogValues(MatrixWorkspace_sptr ws, const std::string &name,
                     const int msize, double value) {
    TimeSeriesProperty<double> *log =
        ws->run().getTimeSeriesProperty<double>(name);
    TS_ASSERT_EQUALS(log->size(), msize);
    std::vector<DateAndTime> mergedtimes = log->timesAsVector();
    for (int i = 0; i < msize; ++i) {
      DateAndTime logtime = mergedtimes[i];
      const double logvalue = log->getSingleValue(logtime);
      TSM_ASSERT_DELTA(std::to_string(i), logvalue, value, 1.e-9);
    }
    TS_ASSERT_EQUALS(log->units(), "TimeOfFligths")
  }

  // msize1 < msize2!
  void testLogValues(MatrixWorkspace_sptr ws, const std::string &name,
                     const int msize1, const int msize2, const double v1,
                     const double v2) {
    TimeSeriesProperty<double> *log =
        ws->run().getTimeSeriesProperty<double>(name);
    TS_ASSERT_EQUALS(log->size(), msize1 + msize2);
    std::vector<DateAndTime> mergedtimes = log->timesAsVector();
    const std::vector<double> v12 = {v1, v2, v1, v2, v1, v2, v1, v2, v1, v2,
                                     v1, v2, v1, v2, v1, v2, v1, v2, v1, v2};
    for (int i = 0; i < msize1 * 2; ++i) {
      DateAndTime logtime = mergedtimes[i];
      const double logvalue = log->getSingleValue(logtime);
      TSM_ASSERT_DELTA(std::to_string(i), logvalue, v12[i], 1.e-9);
    }
    for (int i = msize1 * 2; i < msize2; ++i) {
      DateAndTime logtime = mergedtimes[i];
      const double logvalue = log->getSingleValue(logtime);
      TSM_ASSERT_DELTA(std::to_string(i), logvalue, v2, 1.e-9);
    }
  }
};

#endif /* MANTID_ALGORITHMS_MERGELOGSTEST_H_ */
