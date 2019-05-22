// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_DELETELOGTEST_H_
#define MANTID_ALGORITHMS_DELETELOGTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/DeleteLog.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

class DeleteLogTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static DeleteLogTest *createSuite() { return new DeleteLogTest(); }
  static void destroySuite(DeleteLogTest *suite) { delete suite; }

  // -------------------------- Success tests --------------------------

  void test_Init() {
    Mantid::Algorithms::DeleteLog alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_non_existant_log_does_not_throw_error() {
    Mantid::Algorithms::DeleteLog alg;
    alg.initialize();
    alg.setChild(true); // no ADS storage
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Name", "NotALog"));
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    alg.setProperty("Workspace", ws);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void test_single_value_log_is_deleted() {
    Mantid::Algorithms::DeleteLog alg;
    alg.initialize();
    alg.setChild(true); // no ADS storage
    alg.setRethrows(true);
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    std::string logName("SingleValue");
    ws->mutableRun().addProperty<double>(logName, 1.0);
    alg.setProperty("Workspace", ws);

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Name", logName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(!ws->run().hasProperty(logName));
  }

  void test_time_series_log_is_deleted() {
    Mantid::Algorithms::DeleteLog alg;
    alg.initialize();
    alg.setChild(true); // no ADS storage
    alg.setRethrows(true);
    auto ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    std::string logName("TimeSeries");

    auto *tsp = new Mantid::Kernel::TimeSeriesProperty<double>(logName);
    tsp->addValue("2010-09-14T04:20:12", 20.0);
    ws->mutableRun().addProperty(tsp);
    alg.setProperty("Workspace", ws);

    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Name", logName));
    TS_ASSERT_THROWS_NOTHING(alg.execute());

    TS_ASSERT(!ws->run().hasProperty(logName));
  }

  // -------------------------- Failure tests --------------------------

  void test_empty_log_name_throws_invalid_argument() {
    Mantid::Algorithms::DeleteLog alg;
    alg.initialize();
    TS_ASSERT_THROWS(alg.setProperty("Name", ""), const std::invalid_argument &);
  }
};

#endif /* MANTID_ALGORITHMS_DELETELOGTEST_H_ */
