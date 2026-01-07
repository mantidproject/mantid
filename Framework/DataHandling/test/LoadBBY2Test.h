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
#include "MantidDataHandling/LoadBBY2.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;

class LoadBBY2Test : public CxxTest::TestSuite {
public:
  void test_load_bby2_algorithm_init() {
    std::cout << "\nTesting LoadBBY2 algorithm initialization" << std::endl;
    LoadBBY2 algToBeTested;

    TS_ASSERT_THROWS_NOTHING(algToBeTested.initialize());
    TS_ASSERT(algToBeTested.isInitialized());
  }

  void test_load_bby2_algorithm() {
    std::cout << "\nTesting LoadBBY2 algorithm execution" << std::endl;
    LoadBBY2 algToBeTested;

    if (!algToBeTested.isInitialized())
      algToBeTested.initialize();

    std::string outputSpace = "LoadBBY2Test";
    algToBeTested.setPropertyValue("OutputWorkspace", outputSpace);

    // should fail because mandatory parameter has not been set
    TS_ASSERT_THROWS(algToBeTested.execute(), const std::runtime_error &);

    // should succeed now
    std::string inputFile = "BBY0081723.nxs";
    algToBeTested.setPropertyValue("Filename", inputFile);
    TS_ASSERT_THROWS_NOTHING(algToBeTested.execute());
    TS_ASSERT(algToBeTested.isExecuted());

    // get workspace generated
    MatrixWorkspace_sptr output = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputSpace);

    // check number of histograms
    TS_ASSERT_EQUALS(output->getNumberHistograms(), 61440);
    double sum = 0.0;
    for (size_t i = 0; i < output->getNumberHistograms(); i++)
      sum += output->y(i)[0];
    TS_ASSERT_EQUALS(sum, 804379);

    // check that all required log values are there
    auto run = output->run();

    // test some data properties as well
    auto logpm = [&run](const std::string &tag) {
      TS_ASSERT(run.hasProperty(tag));
      auto *p = dynamic_cast<TimeSeriesProperty<double> *>(run.getProperty(tag));
      TS_ASSERT(p != nullptr);
      return p ? p->firstValue() : 0.0;
    };
    TS_ASSERT_DELTA(logpm("L1"), 6.756, 1.0e-3);
    TS_ASSERT_DELTA(logpm("detector_time"), 299.4, 1.0e-1);
    TS_ASSERT_DELTA(logpm("L2_det_value"), 7.023, 1.0e-3);
    TS_ASSERT_DELTA(logpm("Ltof_det_value"), 25.444, 1.0e-3);
    AnalysisDataService::Instance().remove(outputSpace);
    std::cout << "LoadBBY2Test completed successfully." << std::endl;
  }
};
