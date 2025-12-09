// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidAlgorithms/AddLogInterpolated.h"
#include "MantidDataObjects/Workspace2D.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidKernel/Strings.h"
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/Timer.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Kernel::PropertyWithValue;
using Mantid::Kernel::TimeSeriesProperty;

class AddLogInterpolatedTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddLogInterpolatedTest *createSuite() { return new AddLogInterpolatedTest(); }
  static void destroySuite(AddLogInterpolatedTest *suite) { delete suite; }

  void tearDown() override {
    auto &ads = AnalysisDataService::Instance();
    if (ads.doesExist("_interpolated_test")) {
      ads.remove("_interpolated_test");
    }
    if (ads.doesExist("_tab")) {
      ads.remove("_tab");
    }
  }

  void test_Init() {
    AddLogInterpolated alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_invalid_wksp() {
    AddLogInterpolated alg;
    alg.initialize();

    // setting the input requires workspace in ADS
    TS_ASSERT_THROWS_ASSERT(alg.setProperty("Workspace", "nothing"), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "Analysis Data Service")));
    // set with a table workspace -- important quality is that NOT matrix workspace
    ITableWorkspace_sptr tab = WorkspaceCreationHelper::createEPPTableWorkspace();
    AnalysisDataService::Instance().addOrReplace("_tab", tab);
    TS_ASSERT_THROWS_ASSERT(alg.setProperty("Workspace", "_tab"), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "MatrixWorkspace")));
    // set with a workspace2d
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    alg.setProperty("Workspace", ws);

    // trying to set the logname empty creates an error
    TS_ASSERT_THROWS_ASSERT(alg.setProperty("LogToInterpolate", ""), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "Invalid value for property LogToInterpolate")));
    TS_ASSERT_THROWS_ASSERT(alg.setProperty("LogToMatch", ""), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "Invalid value for property LogToMatch")));
    // the log is not present, so validation will fail
    alg.setProperty("LogToInterpolate", "nonexistent_log");
    alg.setProperty("LogToMatch", "nonexistent_log");
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Log nonexistent_log not found")));

    // set with a non-tsp log
    PropertyWithValue<double> *pwv1 = new PropertyWithValue<double>("pwv_log1", 0);
    PropertyWithValue<double> *pwv2 = new PropertyWithValue<double>("pwv_log2", 0);
    ws->mutableRun().addProperty(pwv1);
    ws->mutableRun().addProperty(pwv2);
    alg.setProperty("LogToInterpolate", "pwv_log1");
    alg.setProperty("LogToMatch", "pwv_log2");
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Log pwv_log1 must be a numerical time series")));
  }

  Workspace2D_sptr make_ws_with_tsp_log(std::vector<double> const &values, double const dx) {
    // setup a workspace with a time series log
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    AnalysisDataService::Instance().addOrReplace("_interpolated_test", ws);
    Mantid::Types::Core::DateAndTime root_time("2016-11-20T16:17");
    TimeSeriesProperty<double> *tspInterp = new TimeSeriesProperty<double>("tsp_interp");
    TimeSeriesProperty<double> *tspMatch = new TimeSeriesProperty<double>("tsp_match");
    for (unsigned i = 0; i < values.size(); i++) {
      tspInterp->addValue((root_time + double(i)), values[i]);
      tspMatch->addValue((root_time + double(i) + dx), 0.0);
    }
    ws->mutableRun().addProperty(tspInterp);
    ws->mutableRun().addProperty(tspMatch);
    return ws;
  }

  void test_execute_interpolate() {
    // setup a workspace with a time series log
    struct {
      double slope = 1.7;
      double intercept = 0.2;
      double operator()(double i) { return slope * i + intercept; }
    } func;
    double dx = 0.2;
    std::vector<double> values(5), expect(5);
    for (int i = 0; i < 5; i++) {
      values[i] = func(i);
    }
    Workspace2D_sptr ws = make_ws_with_tsp_log(values, dx);
    auto *tsp = dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getProperty("tsp_match"));
    auto x = tsp->timesAsVectorSeconds();
    for (int i = 0; i < 5; i++) {
      expect[i] = func(x[i]);
    }

    // setup the algorithm for cubic spline interpolation
    AddLogInterpolated alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogToInterpolate", "tsp_interp"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogToMatch", "tsp_match"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto outTSP = dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getProperty("tsp_interp_interpolated"));
    auto result = outTSP->valuesAsVector();
    TS_ASSERT_EQUALS(result, expect);
  }
};
