// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Run.h"
#include "MantidDataHandling/LoadPLNnxs.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadPLNnxsTest : public CxxTest::TestSuite {
public:
  void test_load_pln2_algorithm_init() {
    LoadPLNnxs algToBeTested;

    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void test_load_pln2_algorithm() {
    LoadPLNnxs algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadPLNnxsTest";

    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // missing event file - should fail execution
    std::string inputFile = "PLN0136193.nxs";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("CalibrateTOFBias", "1");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 12808);
    int sum = 0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += static_cast<int>(output->y(i)[0]);
    TS_ASSERT_EQUALS(sum, 9692);

    // check that all required log values are there
    auto run = output->run();

    // test some data properties
    auto logpm = [&run](const std::string &tag) {
      return dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag))->firstValue();
    };

    double duration = std::stod(run.getProperty("dur")->value());
    TS_ASSERT_DELTA(duration, 60.53, 1.0e-2);
    TS_ASSERT_DELTA(logpm("GatePeriod"), 5000.0, 0.1);
    TS_ASSERT_DELTA(logpm("DetectorTankAngle"), 58.216, 1.0e-3);
    TS_ASSERT_DELTA(logpm("TOFCorrection"), -132.316, 1.0e-3);
    TS_ASSERT_DELTA(logpm("Wavelength"), 4.6900, 1.0e-4);
    TS_ASSERT_DELTA(logpm("SampleRotation"), -18.000, 1.0e-3);

    // check the env parameters
    TS_ASSERT_DELTA(logpm("env_T01S00"), 166.217, 1.0e-3);
    TS_ASSERT_DELTA(logpm("env_T02S00"), 1.199, 1.0e-3);
    TS_ASSERT_THROWS(logpm("env_T3S1"), const std::runtime_error &);
  }

  void test_lambda_on_two_mode() {
    LoadPLNnxs algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadPLNnxsTest";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // set lambda on two to confirm data is processed using half wavelength
    // as the file is not lambda on two data so the TOF calibration is not
    // meaningful
    std::string inputFile = "PLN0136193.nxs";
    algToBeTested.setPropertyValue("Filename", inputFile);
    algToBeTested.setPropertyValue("CalibrateTOFBias", "0");
    algToBeTested.setPropertyValue("TimeOfFlightBias", "-132.0");
    algToBeTested.setPropertyValue("LambdaOnTwoMode", "1");
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 12808);
    int sum = 0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += static_cast<int>(output->y(i)[0]);
    TS_ASSERT_EQUALS(sum, 9692);

    // check that all required log values are there
    auto run = output->run();

    // test some data properties
    auto logpm = [&run](const std::string &tag) {
      return dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag))->firstValue();
    };
    // test duration
    double duration = std::stod(run.getProperty("dur")->value());
    TS_ASSERT_DELTA(duration, 60.53, 1.0e-2);
    TS_ASSERT_DELTA(logpm("GatePeriod"), 5000.0, 0.1);
    TS_ASSERT_DELTA(logpm("DetectorTankAngle"), 58.216, 1.0e-3);
    TS_ASSERT_DELTA(logpm("TOFCorrection"), -132.0, 1.0e-3);
    TS_ASSERT_DELTA(logpm("Wavelength"), 2.3450, 1.0e-4);
    TS_ASSERT_DELTA(logpm("SampleRotation"), -18.000, 1.0e-3);
  }
};
