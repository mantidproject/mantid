// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef FUNCTIONVALUESTEST_H_
#define FUNCTIONVALUESTEST_H_

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

class FunctionValuesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FunctionValuesTest *createSuite() { return new FunctionValuesTest(); }
  static void destroySuite(FunctionValuesTest *suite) { delete suite; }

  FunctionValuesTest() {
    x.resize(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 1.0 + 0.1 * double(i);
    }
  }

  void testCalculatedValues() {
    FunctionDomain1DVector domain(x);
    FunctionValues values(domain);
    TS_ASSERT_EQUALS(values.size(), domain.size());
    for (size_t i = 0; i < values.size(); ++i) {
      TS_ASSERT_EQUALS(values.getCalculated(i), 0.0);
      values.setCalculated(i, double(i) + 0.01);
    }
    for (size_t i = 0; i < values.size(); ++i) {
      TS_ASSERT_EQUALS(values.getCalculated(i), double(i) + 0.01);
      TS_ASSERT_EQUALS(values.getCalculated(i),
                       *values.getPointerToCalculated(i));
    }
  }

  void testPlusOperator() {
    FunctionDomain1DVector domain(x);
    FunctionValues values1(domain);
    FunctionValues values2(domain);

    for (size_t i = 0; i < values1.size(); ++i) {
      values1.setCalculated(i, double(i));
      values2.setCalculated(i, double(i));
    }

    values2 += values1;

    for (size_t i = 0; i < values2.size(); ++i) {
      TS_ASSERT_EQUALS(values2.getCalculated(i), 2.0 * double(i));
    }

    std::vector<double> x3(9);
    FunctionDomain1DVector domain3(x3);
    FunctionValues values3(domain3);

    TS_ASSERT_THROWS(values3 += values1, const std::runtime_error &);
  }

  void testFitData() {
    FunctionDomain1DVector domain(x);
    FunctionValues values1(domain);

    TS_ASSERT_THROWS(values1.getFitData(0), const std::runtime_error &);
    TS_ASSERT_THROWS(values1.getFitWeight(0), const std::runtime_error &);

    values1.setFitData(5, 10.1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_EQUALS(values1.getFitData(i), i == 5 ? 10.1 : 0.0);
      TS_ASSERT_THROWS(values1.getFitWeight(i), const std::runtime_error &);
    }

    std::vector<double> y(9);
    TS_ASSERT_THROWS(values1.setFitData(y), const std::invalid_argument &);

    y.resize(10);
    for (size_t i = 0; i < y.size(); ++i) {
      y[i] = double(2 * i);
    }

    values1.setFitData(y);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_EQUALS(values1.getFitData(i), double(2 * i));
    }
  }

  void testFitWeights() {
    FunctionDomain1DVector domain(x);
    FunctionValues values1(domain);

    values1.setFitWeight(5, 10.1);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_EQUALS(values1.getFitWeight(i), i == 5 ? 10.1 : 0.0);
      TS_ASSERT_THROWS(values1.getFitData(i), const std::runtime_error &);
    }

    std::vector<double> y(9);
    TS_ASSERT_THROWS(values1.setFitWeights(y), const std::invalid_argument &);

    y.resize(10);
    for (size_t i = 0; i < y.size(); ++i) {
      y[i] = double(2 * i);
    }

    values1.setFitWeights(y);
    for (size_t i = 0; i < values1.size(); ++i) {
      TS_ASSERT_EQUALS(values1.getFitWeight(i), double(2 * i));
    }

    FunctionValues values2(domain);
    values2.setFitWeights(100.0);
    for (size_t i = 0; i < values2.size(); ++i) {
      TS_ASSERT_EQUALS(values2.getFitWeight(i), 100.0);
    }
  }

private:
  std::vector<double> x;
};

#endif /*FUNCTIONVALUESTEST_H_*/
