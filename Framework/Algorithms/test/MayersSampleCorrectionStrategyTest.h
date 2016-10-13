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
    Histogram histo(Points(2, LinearGenerator(0, 1)), Counts(2, 0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);
    double absFactor = mscat.calculateSelfAttenuation(0.01);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00030887, absFactor, delta);
  }

  void
  test_Multiple_Scattering_With_Fixed_Mur_And_Absorption_Correction_Factor() {
    Histogram histo(Points(2, LinearGenerator(0, 1)), Counts(2, 0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);
    const size_t irp(1);
    const double muR(0.01), abs(0.0003);
    auto absFactor = mscat.calculateMS(irp, muR, abs);

    const double delta = 1e-8;
    TS_ASSERT_DELTA(0.00461391, absFactor.first, delta);
    TS_ASSERT_DELTA(67.25351289, absFactor.second, delta);
  }

  void test_Corrects_Both_Absorption_And_Multiple_Scattering_For_Point_Data() {
    using std::sqrt;
    const size_t nypts(100);
    std::vector<double> signal(nypts, 2.0), tof(nypts), error(nypts);

    std::transform(signal.begin(), signal.end(), error.begin(),
                   (double (*)(double))sqrt);
    std::generate(tof.begin(), tof.end(), Incrementer(100.0));

    Points tofPoints(tof);
    Histogram histo(tofPoints, Counts(signal), CountStandardDeviations(error));

    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);

    // Correct it

    auto outHisto = mscat.getCorrectedHisto();

    const auto &tofVals = outHisto.x();
    const auto &signalVals = outHisto.y();
    const auto &errVals = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(100.0, tofVals.front(), delta);
    TS_ASSERT_DELTA(199.0, tofVals.back(), delta);

    TS_ASSERT_DELTA(0.37497317, signalVals.front(), delta);
    TS_ASSERT_DELTA(0.37629282, signalVals.back(), delta);

    TS_ASSERT_DELTA(0.26514607, errVals.front(), delta);
    TS_ASSERT_DELTA(0.2660792, errVals.back(), delta);
  }

  void
  test_Corrects_Both_Absorption_And_Multiple_Scattering_For_Histogram_Data() {
    using std::sqrt;
    const size_t nypts(100);
    std::vector<double> signal(nypts, 2.0), tof(nypts + 1), error(nypts);
    std::transform(signal.begin(), signal.end(), error.begin(),
                   (double (*)(double))sqrt);
    // Generate a histogram with the same mid points as the point data example
    std::generate(tof.begin(), tof.end(), Incrementer(99.5));

    BinEdges tofPoints(tof);
    Histogram histo(tofPoints, Counts(signal), CountStandardDeviations(error));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);

    // Correct it

    auto outHisto = mscat.getCorrectedHisto();

    const auto &tofVals = outHisto.x();
    const auto &signalVals = outHisto.y();
    const auto &errVals = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tofVals.front(), delta);
    TS_ASSERT_DELTA(199.5, tofVals.back(), delta);

    TS_ASSERT_DELTA(0.37497317, signalVals.front(), delta);
    TS_ASSERT_DELTA(0.37629281, signalVals.back(), delta);

    TS_ASSERT_DELTA(0.26514607, errVals.front(), delta);
    TS_ASSERT_DELTA(0.26607920, errVals.back(), delta);
  }

  void test_Corrects_For_Absorption_For_Histogram_Data() {
    using std::sqrt;
    const size_t nypts(100);
    std::vector<double> signal(nypts, 2.0), tof(nypts + 1), error(nypts);
    std::transform(signal.begin(), signal.end(), error.begin(),
                   (double (*)(double))sqrt);
    // Generate a histogram with the same mid points as the point data example
    std::generate(tof.begin(), tof.end(), Incrementer(99.5));
    bool mscatOn(false);

    BinEdges tofPoints(tof);
    Histogram histo(tofPoints, Counts(signal), CountStandardDeviations(error));

    MayersSampleCorrectionStrategy mscat(createTestParameters(mscatOn), histo);

    // Correct it

    auto outHisto = mscat.getCorrectedHisto();

    auto tofVals = outHisto.x();
    auto signalVals = outHisto.y();
    auto errVals = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tofVals.front(), delta);
    TS_ASSERT_DELTA(199.5, tofVals.back(), delta);

    TS_ASSERT_DELTA(2.3440378, signalVals.front(), delta);
    TS_ASSERT_DELTA(2.3489418, signalVals.back(), delta);

    TS_ASSERT_DELTA(1.6574850, errVals.front(), delta);
    TS_ASSERT_DELTA(1.6609527, errVals.back(), delta);
  }

  // ---------------------- Failure tests -----------------------------
  void test_Tof_Not_Monotonically_Increasing_Throws_Invalid_Argument() {
    using std::sqrt;
    const size_t nypts(10);
    std::vector<double> signal(nypts, 2.0), tof(nypts), error(nypts);
    std::transform(signal.begin(), signal.end(), error.begin(),
                   (double (*)(double))sqrt);
    std::generate(tof.begin(), tof.end(), Decrementer(199.5));

    Points tofPoints(tof);
    Histogram histo(tofPoints, Counts(signal), CountStandardDeviations(error));

    TS_ASSERT_THROWS(
        MayersSampleCorrectionStrategy(createTestParameters(), histo),
        std::invalid_argument);
  }

private:
  struct Incrementer {
    Incrementer(double start) : current(start) {}
    double operator()() { return current++; }
    double current;
  };
  struct Decrementer {
    Decrementer(double start) : current(start) {}
    double operator()() { return current--; }
    double current;
  };

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
