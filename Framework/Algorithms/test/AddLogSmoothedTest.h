// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidAlgorithms/AddLogSmoothed.h"
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

class AddLogSmoothedTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static AddLogSmoothedTest *createSuite() { return new AddLogSmoothedTest(); }
  static void destroySuite(AddLogSmoothedTest *suite) { delete suite; }

  void tearDown() override {
    auto &ads = AnalysisDataService::Instance();
    if (ads.doesExist("_smooth_test")) {
      ads.remove("_smooth_test");
    }
    if (ads.doesExist("_tab")) {
      ads.remove("_tab");
    }
  }

  void test_Init() {
    AddLogSmoothed alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_invalid_params() {
    AddLogSmoothed alg;
    alg.initialize();

    // set mandatory properties so rest of validation won't fail
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "nonexistent_log"));

    // boxcar smoothing params
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "BoxCar"));
    // must have params passed
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", ""));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Boxcar smoothing requires the window width")));
    // param must be an odd integer
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "2"));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Boxcar smoothing requires an odd")));

    // fft zero smoothing
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "Zeroing"));
    // must have params passed
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", ""));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "FFT zeroing requires the cutoff frequency")));
    // param must be bigger than 1
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "1"));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "The cutoff in FFT zeroing must be larger than 1")));

    // fft butterworth smoothing
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "Butterworth"));
    // must have TWO params passed
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", ""));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Butterworth smoothing requires two parameters")));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "1"));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Butterworth smoothing requires two parameters")));
    // cutoff must be bigger than 1
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "1, 2"));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "cutoff must be greater than 1")));
    // order must be bigger than 0
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "2, 0"));
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "order must be greater than 0")));
  }

  void test_invalid_wksp() {
    AddLogSmoothed alg;
    alg.initialize();

    // set the smoothing params to be valid
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "BoxCar"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "5"));

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
    TS_ASSERT_THROWS_ASSERT(alg.setProperty("LogName", ""), std::invalid_argument const &e,
                            TS_ASSERT(strstr(e.what(), "Invalid value for property LogName")));
    // the log is not present, so validation will fail
    alg.setProperty("LogName", "nonexistent_log");
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Log nonexistent_log not found")));

    // set with a non-tsp log
    PropertyWithValue<double> *pwv = new PropertyWithValue<double>("pwv_log", 0);
    ws->mutableRun().addProperty(pwv);
    alg.setProperty("LogName", "pwv_log");
    TS_ASSERT_THROWS_ASSERT(alg.execute(), std::runtime_error const &e,
                            TS_ASSERT(strstr(e.what(), "Log pwv_log must be a numerical time series")));
  }

  Workspace2D_sptr make_ws_with_tsp_log(std::vector<double> values) {
    // setup a workspace with a time series log
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(1, 1);
    AnalysisDataService::Instance().addOrReplace("_smooth_test", ws);
    Mantid::Types::Core::DateAndTime root_time("2016-11-20T16:17");
    TimeSeriesProperty<double> *tsp = new TimeSeriesProperty<double>("tsp_log");
    for (unsigned i = 0; i < values.size(); i++) {
      tsp->addValue((root_time + double(i)), values[i]);
    }
    ws->mutableRun().addProperty(tsp);
    return ws;
  }

  void test_execute_boxcar() {
    // setup a workspace with a time series log
    std::vector<double> values{1.0, 2.0, 6.0, 4.0};
    std::vector<double> expect{1.5, 3.0, 4.0, 5.0}; // using boxcar window 3
    Workspace2D_sptr ws = make_ws_with_tsp_log(values);

    // setup the algorithm for boxcar smoothing
    AddLogSmoothed alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", ws->getName()));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "tsp_log"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "BoxCar"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "3"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto outTSP = dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getProperty("tsp_log_smoothed"));
    auto result = outTSP->valuesAsVector();
    TS_ASSERT_EQUALS(result, expect);
  }

  void test_execute_fft() {
    // setup a workspace with a time series log
    std::vector<double> values{1, 2, 3, 4, 5};
    Workspace2D_sptr ws = make_ws_with_tsp_log(values);

    // setup the algorithm for fft smoothing
    AddLogSmoothed alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", "_smooth_test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "tsp_log"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "Zeroing"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "3"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto outTSP = dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getProperty("tsp_log_smoothed"));
    TS_ASSERT(outTSP != nullptr);
    auto result = outTSP->valuesAsVector();
    TS_ASSERT_EQUALS(result.size(), values.size());
  }

  void test_execute_butterworth() {
    // setup a workspace with a time series log
    std::vector<double> values{1, 2, 3, 4, 5};
    Workspace2D_sptr ws = make_ws_with_tsp_log(values);

    // setup the algorithm for butterworth smoothing
    AddLogSmoothed alg;
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Workspace", "_smooth_test"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("LogName", "tsp_log"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("SmoothingMethod", "Butterworth"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("Params", "3, 2"));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());

    auto outTSP = dynamic_cast<TimeSeriesProperty<double> *>(ws->run().getProperty("tsp_log_smoothed"));
    TS_ASSERT(outTSP != nullptr);
    auto result = outTSP->valuesAsVector();
    TS_ASSERT_EQUALS(result.size(), values.size());
  }
};
