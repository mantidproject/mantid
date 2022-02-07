// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAlgorithms/CreateWorkspace.h"
#include "MantidAlgorithms/DeleteWorkspace.h"
#include "MantidAlgorithms/GetQsInQENSData.h"

using namespace Mantid;
using namespace Mantid::Algorithms;
using namespace Mantid::API;

class GetQsInQENSDataTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static GetQsInQENSDataTest *createSuite() { return new GetQsInQENSDataTest(); }
  static void destroySuite(GetQsInQENSDataTest *suite) { delete suite; }

  /*
   * Tests initializing the GetQsInQENSData algorithm
   */
  void testInit() {
    GetQsInQENSData alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
  }

  /*
   * Tests if the correct error message is generated from executing the
   * GetQsInQENSData algorithm with an input workspace without detectors
   */
  void testNoDetectors() {
    const std::string outputWsName = "data";
    const std::string verticalAxisUnit = "Label";
    const std::vector<double> dataX = {0, 1};
    const std::vector<double> dataY = {0, 0};
    const std::vector<std::string> verticalAxisValues = {"0", "1"};
    const int numSpectra = 2;

    // Create the input workspace, without detectors
    CreateWorkspace createAlg;
    createAlg.initialize();
    createAlg.setProperty("OutputWorkspace", outputWsName);
    createAlg.setProperty("DataX", dataX);
    createAlg.setProperty("DataY", dataY);
    createAlg.setProperty("NSpec", numSpectra);
    createAlg.setProperty("VerticalAxisUnit", verticalAxisUnit);
    createAlg.setProperty("VerticalAxisValues", verticalAxisValues);
    createAlg.execute();

    MatrixWorkspace_sptr workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(outputWsName);
    TS_ASSERT(workspace);
    const std::string expectedErrorMsg = "Detectors are missing from the input workspace";

    try {
      GetQsInQENSData alg;
      alg.initialize();
      alg.setProperty("InputWorkspace", workspace);
      alg.setProperty("RaiseMode", true);
      alg.execute();
    } catch (std::exception &e) {
      std::string errorMsg = e.what();
      TS_ASSERT(errorMsg.find(expectedErrorMsg) != std::string::npos);
    }
  }
};
