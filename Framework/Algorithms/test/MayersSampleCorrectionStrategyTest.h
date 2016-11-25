#ifndef MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGYTEST_H_
#define MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrectionStrategy.h"
#include "MantidHistogramData/LinearGenerator.h"
#include <algorithm>
#include <cmath>

using Mantid::Algorithms::MayersSampleCorrectionStrategy;
using namespace Mantid::HistogramData;

class MayersSampleCorrectionStrategyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MayersSampleCorrectionStrategyTest *createSuite() {
    return new MayersSampleCorrectionStrategyTest();
  }
  static void destroySuite(MayersSampleCorrectionStrategyTest *suite) {
    delete suite;
  }

  void test_Attentuaton_Correction_For_Fixed_Mur() {
    Histogram histo(Points{0, 1}, Counts{0, 1});
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);
    auto absFactor = mscat.calculateSelfAttenuation(0.01);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00030887, absFactor, delta);
  }

  void
  test_Multiple_Scattering_With_Fixed_Mur_And_Absorption_Correction_Factor() {
    Histogram histo(Points{0, 1}, Counts{0, 1});
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);
    const size_t irp(1);
    const double muR(0.01), abs(0.0003);
    auto absFactor = mscat.calculateMS(irp, muR, abs);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00461391, absFactor.first, delta);
    TS_ASSERT_DELTA(67.25351289, absFactor.second, delta);
  }

  void test_Corrects_Both_Absorption_And_Multiple_Scattering_For_Point_Data() {
    const size_t nypts(100);
    Histogram histo(Points(nypts, LinearGenerator(100.0, 1.0)),
                    Counts(nypts, 2.0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);

    auto outHisto = mscat.getCorrectedHisto();

    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(100.0, tof.front(), delta);
    TS_ASSERT_DELTA(199.0, tof.back(), delta);

    TS_ASSERT_DELTA(0.37497317, signal.front(), delta);
    TS_ASSERT_DELTA(0.37629282, signal.back(), delta);

    TS_ASSERT_DELTA(0.26514607, error.front(), delta);
    TS_ASSERT_DELTA(0.2660792, error.back(), delta);
  }

  void
  test_Corrects_Both_Absorption_And_Multiple_Scattering_For_Histogram_Data() {
    const size_t nypts(100);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(99.5, 1.0)),
                    Counts(nypts, 2.0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);

    auto outHisto = mscat.getCorrectedHisto();

    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(0.37497317, signal.front(), delta);
    TS_ASSERT_DELTA(0.37629282, signal.back(), delta);

    TS_ASSERT_DELTA(0.26514607, error.front(), delta);
    TS_ASSERT_DELTA(0.2660792, error.back(), delta);
  }

  void test_Corrects_For_Absorption_For_Histogram_Data() {
    const size_t nypts(100);
    bool mscatOn(false);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(99.5, 1.0)),
                    Counts(nypts, 2.0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(mscatOn), histo);

    auto outHisto = mscat.getCorrectedHisto();

    auto tof = outHisto.x();
    auto signal = outHisto.y();
    auto error = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.3440379, signal.front(), delta);
    TS_ASSERT_DELTA(2.3489418, signal.back(), delta);

    TS_ASSERT_DELTA(1.6574851, error.front(), delta);
    TS_ASSERT_DELTA(1.6609527, error.back(), delta);
  }

  // ---------------------- Failure tests -----------------------------
  void test_Tof_Not_Monotonically_Increasing_Throws_Invalid_Argument() {
    const size_t nypts(10);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(199.5, -1.0)),
                    Counts(nypts, 2.0));

    TS_ASSERT_THROWS(
        MayersSampleCorrectionStrategy(createTestParameters(), histo),
        std::invalid_argument);
  }

private:
  MayersSampleCorrectionStrategy::Parameters
  createTestParameters(bool mscatOn = true) {
    // A bit like a POLARIS spectrum
    MayersSampleCorrectionStrategy::Parameters pars;
    pars.mscat = mscatOn;
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

#endif /* MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGYTEST_H_ */
