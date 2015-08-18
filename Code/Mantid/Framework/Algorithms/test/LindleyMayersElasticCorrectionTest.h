#ifndef MANTID_ALGORITHMS_LINDLEYMAYERSELASTICCORRECTIONTEST_H_
#define MANTID_ALGORITHMS_LINDLEYMAYERSELASTICCORRECTIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/MultipleScattering/LindleyMayersElasticCorrection.h"
#include <algorithm>
#include <cmath>

using Mantid::Algorithms::LindleyMayersElasticCorrection;
using Mantid::Algorithms::ScatteringCorrectionParameters;

class LindleyMayersElasticCorrectionTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static LindleyMayersElasticCorrectionTest *createSuite() {
    return new LindleyMayersElasticCorrectionTest();
  }
  static void destroySuite(LindleyMayersElasticCorrectionTest *suite) {
    delete suite;
  }

  void test_attentuaton_correction_for_fixed_mur() {
    LindleyMayersElasticCorrection mscat(createTestParameters());
    auto absFactor = mscat.calculateSelfAttenuation(0.01);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00030887, absFactor, delta);
  }

  void test_multiple_scattering_with_fixed_mur_and_absorption_correction_factor() {
    LindleyMayersElasticCorrection mscat(createTestParameters());
    const size_t irp(0);
    const double muR(0.01), abs(0.0003);
    auto absFactor = mscat.calculateMS(irp, muR, abs);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00461391, absFactor.first, delta);
    TS_ASSERT_DELTA(67.25351289, absFactor.second, delta);
  }
  
  void xtest_default_corrects_both_absorption_and_multiple_scattering() {
    LindleyMayersElasticCorrection mscat(createTestParameters());
    // Histogram data
    const size_t nypts(100);
    std::vector<double> signal(nypts, 2.0), tof(nypts + 1), error(nypts);
    std::transform(error.begin(), error.end(), error.begin(), sqrt);
    double xcur(100.0);
    std::generate(tof.begin(), tof.end(), [&xcur] { return xcur++; });

    // Correct it
    mscat.apply(tof, signal, error);

    // Check some values
    const double delta(1e-08);
    TS_ASSERT_DELTA(100.0, tof.front(), delta);
    TS_ASSERT_DELTA(110.0, tof.back(), delta);

    TS_ASSERT_DIFFERS(2.0, signal.front());
    TS_ASSERT_DIFFERS(2.0, signal.back());

    TS_ASSERT_DIFFERS(sqrt(2.0), error.front());
    TS_ASSERT_DIFFERS(sqrt(2.0), error.back());
  }

private:
  ScatteringCorrectionParameters createTestParameters() {
    // A bit like a POLARIS spectrum
    ScatteringCorrectionParameters pars = {14.0, 2.2, 0.10821, 0.0, 0.072, 5.08,
                                           5.1,  0.0025, 0.04};
    return pars;
  }
};

#endif /* MANTID_ALGORITHMS_LINDLEYMAYERSELASTICCORRECTIONTEST_H_ */
