#ifndef TOFSANSRESOLUTIONBYPIXELCALCULATORTEST_H_
#define TOFSANSRESOLUTIONBYPIXELCALCULATORTEST_H_

#include "MantidAlgorithms/TOFSANSResolutionByPixelCalculator.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::Algorithms;
class TOFSANSResolutionByPixelCalculatorTest : public CxxTest::TestSuite {
public:
  void test_that_correct_prefactor_is_produced() {
    // Arrange
    const double r1 = 3;
    const double r2 = 2;
    const double deltaR = 6;
    const double lCollim = 1;
    const double l2 = 2;
    TOFSANSResolutionByPixelCalculator calc;

    // Act
    auto result =
        calc.getWavelengthIndependentFactor(r1, r2, deltaR, lCollim, l2);

    // Assert
    double expectedResult = 21;
    TSM_ASSERT_EQUALS("Prefactor should have a value of 21*pi^2",
                      result / M_PI / M_PI, expectedResult);
  }

  void test_that_correct_q_uncertainty_is_calculated() {
    // Arrange
    const double prefactor = 9;
    const double moderatorValue = 1000;
    const double q = 2;
    const double wavelength = 1;
    const double deltaWavelength = 6;
    const double lCollim = 1;
    const double l2 = 2.956;
    TOFSANSResolutionByPixelCalculator calc;

    // Act
    auto result = calc.getSigmaQValue(moderatorValue, prefactor, q, wavelength,
                                      deltaWavelength, lCollim, l2);

    // Assert
    double expectedResult = 5;
    TSM_ASSERT_EQUALS("Should be equate to 5", expectedResult, result);
  }
};
#endif