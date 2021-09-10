// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/Interpolation.h"
#include <ctime>
#include <cxxtest/TestSuite.h>

using namespace Mantid::Kernel;

class InterpolationTest : public CxxTest::TestSuite {
public:
  // This means the constructor isn't called when running other tests
  static InterpolationTest *createSuite() { return new InterpolationTest(); }
  static void destroySuite(InterpolationTest *suite) { delete suite; }

  /* In the constructor some vectors with values are setup,
   * which make the tests easier later on.
   *
   * To check the interpolated values, call the method
   *   checkInterpolationResults(const Interpolation &interpolation);
   * and supply the interpolation object which is to be tested. The method will
   *call
   * further methods to cover all possible edge-cases. On failure, it will be
   *visible
   * which case caused the failure.
   */
  InterpolationTest() {
    // values for setting up the interpolation
    m_tableValues.emplace_back(DataXY(200.0, 50));
    m_tableValues.emplace_back(DataXY(201.0, 60));
    m_tableValues.emplace_back(DataXY(202.0, 100));
    m_tableValues.emplace_back(DataXY(203.0, 300));
    m_tableValues.emplace_back(DataXY(204.0, 400));

    // bulk values for interpolation test
    m_interpolationXValues.emplace_back(200.5);
    m_interpolationXValues.emplace_back(201.25);
    m_interpolationXValues.emplace_back(203.5);

    m_expectedYValues.emplace_back(55.0);
    m_expectedYValues.emplace_back(70.0);
    m_expectedYValues.emplace_back(350.0);

    // values outside interpolation range
    m_outsideXValues.emplace_back(100.0);
    m_outsideXValues.emplace_back(3000.0);

    m_outsideYValues.emplace_back(-950.0);
    m_outsideYValues.emplace_back(280000.0);
  }

  void testCopyConstruction() {
    Interpolation interpolation;
    interpolation.setMethod("linear");
    interpolation.setXUnit("Wavelength");
    interpolation.setYUnit("dSpacing");

    interpolation.addPoint(200.0, 2.0);
    interpolation.addPoint(202.0, 3.0);

    Interpolation other = interpolation;

    TS_ASSERT_EQUALS(other.getMethod(), "linear");
    TS_ASSERT_EQUALS(other.getXUnit()->unitID(), "Wavelength");
    TS_ASSERT_EQUALS(other.getYUnit()->unitID(), "dSpacing");
    TS_ASSERT_EQUALS(other.value(200.0), 2.0);
  }

  void testContainData() {
    Interpolation interpolation;

    TS_ASSERT(interpolation.containData() == false);

    interpolation.addPoint(200.0, 50);

    TS_ASSERT(interpolation.containData() == true);
  }

  void testResetData() {
    Interpolation interpolation = getInitializedInterpolation("Wavelength", "dSpacing");

    TS_ASSERT(interpolation.containData());
    interpolation.resetData();
    TS_ASSERT(interpolation.containData() == false);
  }

  void testAddPointOrdered() {
    Interpolation interpolation;

    // Add points from values in vector in correct order.
    for (size_t i = 0; i < m_tableValues.size(); ++i) {
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
    }

    // Check correctness of interpolation for different cases
    checkInterpolationResults(interpolation);
  }

  void testAddPointArbitrary() {
    Interpolation interpolation;

    size_t insertionOrderRaw[] = {1, 0, 3, 4, 2};
    std::vector<size_t> insertionOrder(insertionOrderRaw, insertionOrderRaw + 5);

    for (std::vector<size_t>::const_iterator i = insertionOrder.begin(); i != insertionOrder.end(); ++i) {
      interpolation.addPoint(m_tableValues[*i].first, m_tableValues[*i].second);
    }

    checkInterpolationResults(interpolation);
  }

  void testAddPointDuplicates() {
    TestableInterpolation interpolation;

    // Add points from values in vector twice.
    for (size_t i = 0; i < m_tableValues.size(); ++i) {
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
    }

    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.cend()), 5);
  }

  void testEmpty() {
    Interpolation interpolation;

    std::stringstream str;
    str << interpolation;
    TS_ASSERT(str.str().compare("linear ; TOF ; TOF") == 0);

    Interpolation readIn;
    str >> readIn;

    TS_ASSERT(readIn.containData() == false);
  }

  void testStreamOperators() {
    std::string xUnit = "Wavelength";
    std::string yUnit = "dSpacing";

    Interpolation interpolation = getInitializedInterpolation(xUnit, yUnit);

    // Output stream
    std::stringstream str;
    str << interpolation;
    TS_ASSERT(str.str().compare("linear ; Wavelength ; dSpacing ; 200 50 ; 201 "
                                "60 ; 202 100 ; 203 300 ; 204 400") == 0);

    // Input stream for empty interpolation object
    Interpolation readIn;
    TS_ASSERT(readIn.getXUnit()->unitID() == "TOF");
    TS_ASSERT(readIn.getYUnit()->unitID() == "TOF");
    str >> readIn;
    TS_ASSERT(readIn.getXUnit()->unitID() == xUnit);
    TS_ASSERT(readIn.getYUnit()->unitID() == yUnit);

    checkInterpolationResults(readIn);
  }

  void testStreamOperatorsNonEmpty() {
    Interpolation interpolation = getInitializedInterpolation("Wavelength", "dSpacing");

    std::stringstream str;
    str << interpolation;

    // Reconstruct on existing object.
    str >> interpolation;

    checkInterpolationResults(interpolation);
  }

  void testFindIndexOfNextLargerValue() {
    TestableInterpolation interpolation;

    // take values from constructor
    for (size_t i = 0; i < m_tableValues.size(); ++i) {
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
    }

    // lower limit - can be treated like general case
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(200.0)), 1);

    // Exact interpolation points
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(201.0)), 2);
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(202.0)), 3);
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(203.0)), 4);

    // Arbitrary interpolation points
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(200.5)), 1);
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(201.25)), 2);
    TS_ASSERT_EQUALS(std::distance(interpolation.cbegin(), interpolation.findIndexOfNextLargerValue(203.5)), 4);
  }

  void testCBegin() {
    TestableInterpolation interpolation;
    // take values from constructor
    for (size_t i = 0; i < m_tableValues.size(); ++i) {
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
    }
    TS_ASSERT_EQUALS(interpolation.cbegin()->first, 200.0)
  }

  void testCEnd() {
    TestableInterpolation interpolation;
    // take values from constructor
    for (size_t i = 0; i < m_tableValues.size(); ++i) {
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
    }
    TS_ASSERT_EQUALS(std::prev(interpolation.cend())->first, 204.0)
  }

  void testInterpolationWithTooFewValues() {
    Interpolation interpolationZero;
    Interpolation interpolationOne;
    interpolationOne.addPoint(200, 2.0);

    for (DataXY m_tableValue : m_tableValues) {
      // When there are zero values in the interpolation, it returns 0.0
      checkValue(interpolationZero, m_tableValue.first, 0.0, "zero interpolation values");

      // With one value, it returns this one value for any x.
      checkValue(interpolationOne, m_tableValue.first, 2.0, "one interpolation value");
    }
  }

private:
  Interpolation getInitializedInterpolation(const std::string &xUnit, const std::string &yUnit) {
    Interpolation interpolation;

    // take values from constructor
    for (size_t i = 0; i < m_tableValues.size(); ++i) {
      interpolation.addPoint(m_tableValues[i].first, m_tableValues[i].second);
    }

    interpolation.setXUnit(xUnit);
    interpolation.setYUnit(yUnit);

    return interpolation;
  }

  void checkInterpolationResults(const Interpolation &interpolation) {
    checkValueAtLowerLimit(interpolation);
    checkValueAtUpperLimit(interpolation);
    checkValuesAtExactBulkPoints(interpolation);
    checkValuesInsideInterpolationRange(interpolation);
    checkValuesOutsideInterpolationRange(interpolation);
  }

  void checkValueAtLowerLimit(const Interpolation &interpolation) {
    checkValue(interpolation, m_tableValues.front().first, m_tableValues.front().second, "at lower limit");
  }

  void checkValueAtUpperLimit(const Interpolation &interpolation) {
    checkValue(interpolation, m_tableValues.back().first, m_tableValues.back().second, "at upper limit");
  }

  void checkValuesAtExactBulkPoints(const Interpolation &interpolation) {
    for (size_t i = 1; i < m_tableValues.size() - 1; ++i) {
      checkValue(interpolation, m_tableValues[i].first, m_tableValues[i].second, "at interpolation point");
    }
  }

  void checkValuesInsideInterpolationRange(const Interpolation &interpolation) {
    for (size_t i = 0; i < m_interpolationXValues.size(); ++i) {
      checkValue(interpolation, m_interpolationXValues[i], m_expectedYValues[i], "inside interpolation range");
    }
  }

  void checkValuesOutsideInterpolationRange(const Interpolation &interpolation) {
    for (size_t i = 0; i < m_outsideXValues.size(); ++i) {
      checkValue(interpolation, m_outsideXValues[i], m_outsideYValues[i], "outside interpolation range");
    }
  }

  /* This function performs the actual check.
   * It takes a string argument to make it more obvious where the problem is.
   */
  void checkValue(const Interpolation &interpolation, double x, double y, const std::string &testedRange) {
    std::ostringstream errorString;
    errorString << "Interpolation error " << testedRange;

    TSM_ASSERT_EQUALS(errorString.str().c_str(), interpolation.value(x), y);
  }

  // This vector contains the data points from which the interpolation is
  // constructed
  std::vector<DataXY> m_tableValues;

  // Two vectors with test values for the "bulk", e.g. no values at the limits
  // and
  std::vector<double> m_interpolationXValues;
  std::vector<double> m_expectedYValues;

  // Values outside interpolation range
  std::vector<double> m_outsideXValues;
  std::vector<double> m_outsideYValues;

  // For the test of findIndexOfNextLargerValue access to protected member is
  // needed
  class TestableInterpolation : public Interpolation {
    friend class InterpolationTest;

  public:
    TestableInterpolation() : Interpolation() {}
    ~TestableInterpolation() override {}
  };
};
