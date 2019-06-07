// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGYTEST_H_
#define MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MayersSampleCorrectionStrategy.h"
#include "MantidHistogramData/LinearGenerator.h"
#include <algorithm>
#include <cmath>

#include <iomanip>

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

  void test_Correction_Skips_Zero_Counts() {
    Histogram histo(Points{2, LinearGenerator(0, 1)},
                    Counts{2, LinearGenerator(0, 1)});
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);
    const auto outHisto = mscat.getCorrectedHisto();

    const auto &yVals = outHisto.y();
    const auto &eVals = outHisto.e();

    TSM_ASSERT_EQUALS("Bin with 0 count was modified", yVals[0], 0);
    TSM_ASSERT_EQUALS("Err val for 0 count was modified", eVals[0], 0);
  }

  void
  test_Multiple_Scattering_With_Fixed_Mur_And_Absorption_Correction_Factor() {
    Histogram histo(Points{0, 1}, Counts{0, 1});
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);
    const size_t irp(1);
    const double muR(0.01), abs(0.0003);
    auto absFactor = mscat.calculateMS(irp, muR, abs);

    const double delta = 1e-6;
    TS_ASSERT_DELTA(0.004671, absFactor.first, delta);
    TS_ASSERT_DELTA(29.258163, absFactor.second, delta);
  }

  void test_Corrects_Both_Absorption_And_Multiple_Scattering_For_Point_Data() {
    const size_t nypts(100);
    Histogram histo(Points(nypts, LinearGenerator(100.0, 1.0)),
                    Counts(nypts, 2.0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);

    const auto outHisto = mscat.getCorrectedHisto();
    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(100.0, tof.front(), delta);
    TS_ASSERT_DELTA(199.0, tof.back(), delta);

    TS_ASSERT_DELTA(2.308089, signal.front(), delta);
    TS_ASSERT_DELTA(2.314809, signal.back(), delta);

    TS_ASSERT_DELTA(1.632065, error.front(), delta);
    TS_ASSERT_DELTA(1.636817, error.back(), delta);
  }

  void
  test_Corrects_Both_Absorption_And_Multiple_Scattering_For_Histogram_Data() {
    const size_t nypts(100);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(99.5, 1.0)),
                    Counts(nypts, 2.0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(), histo);

    const auto outHisto = mscat.getCorrectedHisto();
    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.308089, signal.front(), delta);
    TS_ASSERT_DELTA(2.314809, signal.back(), delta);

    TS_ASSERT_DELTA(1.632065, error.front(), delta);
    TS_ASSERT_DELTA(1.636817, error.back(), delta);
  }

  void test_Corrects_For_Absorption_For_Histogram_Data() {
    const size_t nypts(100);
    const bool mscatOn(false);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(99.5, 1.0)),
                    Counts(nypts, 2.0));
    MayersSampleCorrectionStrategy mscat(createTestParameters(mscatOn), histo);

    auto outHisto = mscat.getCorrectedHisto();

    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.3440379, signal.front(), delta);
    TS_ASSERT_DELTA(2.3489418, signal.back(), delta);

    TS_ASSERT_DELTA(1.6574851, error.front(), delta);
    TS_ASSERT_DELTA(1.6609527, error.back(), delta);
  }

  void test_MutlipleScattering_NEvents_Parameter() {
    const size_t nypts(100);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(99.5, 1.0)),
                    Counts(nypts, 2.0));
    const bool mscatOn(true);
    auto corrPars = createTestParameters(mscatOn);
    corrPars.msNEvents = 1000;
    MayersSampleCorrectionStrategy mscat(corrPars, histo);

    const auto outHisto = mscat.getCorrectedHisto();
    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.307860, signal.front(), delta);
    TS_ASSERT_DELTA(2.314794, signal.back(), delta);

    TS_ASSERT_DELTA(1.631904, error.front(), delta);
    TS_ASSERT_DELTA(1.636807, error.back(), delta);
  }

  void test_MutlipleScattering_NRuns_Parameter() {
    const size_t nypts(100);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(99.5, 1.0)),
                    Counts(nypts, 2.0));
    const bool mscatOn(true);
    auto corrPars = createTestParameters(mscatOn);
    corrPars.msNRuns = 2;
    MayersSampleCorrectionStrategy mscat(corrPars, histo);

    const auto outHisto = mscat.getCorrectedHisto();
    const auto &tof = outHisto.x();
    const auto &signal = outHisto.y();
    const auto &error = outHisto.e();

    // Check some values
    // Check some values
    const double delta(1e-06);
    TS_ASSERT_DELTA(99.5, tof.front(), delta);
    TS_ASSERT_DELTA(199.5, tof.back(), delta);

    TS_ASSERT_DELTA(2.308851, signal.front(), delta);
    TS_ASSERT_DELTA(2.323555, signal.back(), delta);

    TS_ASSERT_DELTA(1.632604, error.front(), delta);
    TS_ASSERT_DELTA(1.643002, error.back(), delta);
  }

  // ---------------------- Failure tests -----------------------------
  void test_Tof_Not_Monotonically_Increasing_Throws_Invalid_Argument() {
    const size_t nypts(10);
    Histogram histo(BinEdges(nypts + 1, LinearGenerator(199.5, -1.0)),
                    Counts(nypts, 2.0));

    TS_ASSERT_THROWS(
        MayersSampleCorrectionStrategy(createTestParameters(), histo),
        const std::invalid_argument &);
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
    pars.azimuth = 0.0;
    pars.rho = 0.07261;
    pars.sigmaSc = 5.1;
    pars.sigmaAbs = 5.08;
    pars.cylRadius = 0.0025;
    pars.cylHeight = 0.04;
    pars.msNEvents = 500;
    pars.msNRuns = 10;
    return pars;
  }
};

#endif /* MANTID_ALGORITHMS_MAYERSSAMPLECORRECTIONSTRATEGYTEST_H_ */
