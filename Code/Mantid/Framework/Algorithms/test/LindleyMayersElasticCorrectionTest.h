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

  void
  test_multiple_scattering_with_fixed_mur_and_absorption_correction_factor() {
    LindleyMayersElasticCorrection mscat(createTestParameters());
    const size_t irp(0);
    const double muR(0.01), abs(0.0003);
    auto absFactor = mscat.calculateMS(irp, muR, abs);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00461391, absFactor.first, delta);
    TS_ASSERT_DELTA(67.25351289, absFactor.second, delta);
  }

  void test_default_corrects_both_absorption_and_multiple_scattering() {
    LindleyMayersElasticCorrection mscat(createTestParameters());
    const size_t nypts(100);
    std::vector<double> signal(nypts, 2.0), tof(nypts), error(nypts);
    std::transform(signal.begin(), signal.end(), error.begin(), sqrt);
    double xcur(100.0);
    std::generate(tof.begin(), tof.end(), [&xcur] { return xcur++; });

    // Correct it
    mscat.apply(tof, signal, error);

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(100.0, tof.front(), delta);
    TS_ASSERT_DELTA(199.0, tof.back(), delta);

    TS_ASSERT_DELTA(-10.406096, signal.front(), delta);
    TS_ASSERT_DELTA(-10.366438, signal.back(), delta);

    TS_ASSERT_DELTA(-7.358221, error.front(), delta);
    TS_ASSERT_DELTA(-7.330179, error.back(), delta);
  }

private:
  ScatteringCorrectionParameters createTestParameters() {
    // A bit like a POLARIS spectrum
    ScatteringCorrectionParameters pars;
    pars.l1 = 14.0;
    pars.l2 = 2.2;
    pars.twoTheta = 0.10821;
    pars.phi = 0.0;
    pars.rho = 0.07261;
    pars.sigmaSc = 5.1;
    pars.sigmaAbs = 5.08;
    pars.cylRadius = 0.0025;
    pars.cylHeight = 0.04;
    return pars;
  }
};

#endif /* MANTID_ALGORITHMS_LINDLEYMAYERSELASTICCORRECTIONTEST_H_ */
