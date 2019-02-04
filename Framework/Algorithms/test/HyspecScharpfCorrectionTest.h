// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_HYSPECSCHARPFCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_HYSPECSCHARPFCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/HyspecScharpfCorrection.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using Mantid::Algorithms::HyspecScharpfCorrection;

class HyspecScharpfCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HyspecScharpfCorrectionTest *createSuite() {
    return new HyspecScharpfCorrectionTest();
  }
  static void destroySuite(HyspecScharpfCorrectionTest *suite) { delete suite; }

  void test_Init() {
    HyspecScharpfCorrection alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_exec() {
    // Create test input
    std::vector<double> L2 = {1.0}, polar = {M_PI_4}, azimuthal = {0.};
    auto inputWS = WorkspaceCreationHelper::createProcessedInelasticWS(
        L2, polar, azimuthal, 30, -10, 20, 17.1);
    HyspecScharpfCorrection alg;

    alg.setChild(true);
    alg.setRethrows(true);
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("InputWorkspace", inputWS));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(
        "OutputWorkspace", "HyspecScharpfCorrectionOutput"));
    TS_ASSERT_THROWS_NOTHING(alg.setProperty("PolarizationAngle", -11.0));
    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());

    // Retrieve the workspace from the algorithm.
    Mantid::API::MatrixWorkspace_sptr outputWS =
        alg.getProperty("OutputWorkspace");
    TS_ASSERT(outputWS);
    auto histo = outputWS->histogram(0);
    auto x = histo.points();
    const auto &y = histo.y();
    for (size_t i = 0; i < x.size(); ++i) {
      if (x[i] < 4) {
        TS_ASSERT_LESS_THAN(y[i], 0);
      } else if (x[i] < 6. || x[i] > 17.) {
        TS_ASSERT_EQUALS(y[i], 0.);
      } else {
        TS_ASSERT_LESS_THAN(0, y[i]);
      }
    }
    // test one value, say DeltaE=6.5
    double kikf = std::sqrt(1. - 6.5 / 17.1);
    auto alpha =
        std::atan2(-kikf * std::sin(M_PI_4), 1. - kikf * std::cos(M_PI_4)) +
        11. * M_PI / 180.;
    TS_ASSERT_DELTA(x[16], 6.5, 1e-10);
    // note that it does the correction factor as a float
    // as it is a common code with events
    TS_ASSERT_DELTA(y[16], 1. / std::cos(2. * alpha), 1e-6);
  }
};

#endif /* MANTID_ALGORITHMS_HYSPECSCHARPFCORRECTIONTEST_H_ */
