// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_BOSEEINSTEINDISTRIBUTIONTEST_H_
#define MANTID_KERNEL_BOSEEINSTEINDISTRIBUTIONTEST_H_

#include "MantidKernel/Math/Distributions/BoseEinsteinDistribution.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cxxtest/TestSuite.h>

class BoseEinsteinDistributionTest : public CxxTest::TestSuite {
public:
  void test_Standard_Distribution_Gives_Correct_Value_Away_From_Edge() {
    using namespace Mantid::Kernel::Math;
    const double energy = 30.0;
    const double temperature = 35.0;

    TS_ASSERT_DELTA(BoseEinsteinDistribution::n(energy, temperature),
                    0.000047886213, 1e-12);
  }

  void test_Standard_Distribution_Throws_When_Energy_Or_Temperature_Is_Zero() {
    using namespace Mantid::Kernel::Math;
    const double energy = 0.0;
    const double temperature = 35.0;

    TS_ASSERT_THROWS(BoseEinsteinDistribution::n(energy, temperature),
                     const std::domain_error &);
    TS_ASSERT_THROWS(BoseEinsteinDistribution::n(temperature, energy),
                     const std::domain_error &);
  }

  void
  test_np1Eps_Is_Returns_Energy_When_Temp_Is_Negative_And_Energy_Positive() {
    using namespace Mantid::Kernel::Math;
    const double energy = 200;
    const double temperature = -35.0;

    const double expected = energy;
    TS_ASSERT_DELTA(BoseEinsteinDistribution::np1Eps(energy, temperature),
                    expected, 1e-12);
  }

  void test_np1Eps_Returns_kbT_When_Answer_When_Exponent_Is_Zero() {
    using namespace Mantid::Kernel::Math;
    const double energy = 0.0;
    const double temperature = 35.0;

    const double expected =
        Mantid::PhysicalConstants::BoltzmannConstant * temperature;
    TS_ASSERT_DELTA(BoseEinsteinDistribution::np1Eps(energy, temperature),
                    expected, 1e-12);
  }

  void test_np1Eps_Is_Returns_Zero_When_Temp_Is_Negative_And_Energy_Negative() {
    using namespace Mantid::Kernel::Math;
    const double energy = -200;
    const double temperature = -35.0;

    const double expected = 0.0;
    TS_ASSERT_DELTA(BoseEinsteinDistribution::np1Eps(energy, temperature),
                    expected, 1e-12);
  }

  void test_np1Eps_Is_Well_Behaved_When_Exponent_Is_Larger_Than_Point1() {
    using namespace Mantid::Kernel::Math;
    const double energy = 20;
    const double temperature = 29.0;

    const double expected = 20.006690611537;

    TS_ASSERT_DELTA(BoseEinsteinDistribution::np1Eps(energy, temperature),
                    expected, 1e-12);
  }

  void
  test_np1Eps_Is_Well_Behaved_When_Abs_Exponent_Is_Larger_Than_Point1_But_Large_And_Negative() {
    using namespace Mantid::Kernel::Math;
    const double energy = -20;
    const double temperature = 35.0;

    const double expected = 0.026407635389;
    TS_ASSERT_DELTA(BoseEinsteinDistribution::np1Eps(energy, temperature),
                    expected, 1e-12);
  }
};

#endif /* MANTID_KERNEL_BOSEEINSTEINDISTRIBUTIONTEST_H_ */
