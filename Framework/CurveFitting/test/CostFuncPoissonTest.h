#ifndef MANTID_CURVEFITTING_COSTFUNCPOISSONTEST_H_
#define MANTID_CURVEFITTING_COSTFUNCPOISSONTEST_H_

#include <cmath>
#include <numeric>

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/CostFunctions/CostFuncPoisson.h"
#include "MantidCurveFitting/Functions/UserFunction.h"
#include "MantidCurveFitting/Jacobian.h"

using namespace Mantid::API;
using Mantid::CurveFitting::CostFunctions::CostFuncPoisson;
using Mantid::CurveFitting::Functions::UserFunction;

namespace {
// Taken from CostFuncPoisson
const double cutOffPoint = 0.0001;

std::vector<double> calculateDeterminant(const FunctionValues &vals,
                                         size_t numParams) {
  Mantid::CurveFitting::Jacobian jacob(vals.size(), numParams);
  std::vector<double> expectedVals(numParams);

  for (size_t iParam = 0; iParam < numParams; iParam++) {

    for (size_t iDataPoint = 0; iDataPoint < vals.size(); iDataPoint++) {
      // The method names are a bit inconsistent with the data
      // they represent
      const double fitted = vals.getCalculated(iDataPoint);
      const double binCounts = vals.getFitData(iDataPoint);

      expectedVals[iParam] =
          jacob.get(iDataPoint, iParam) * (1 - binCounts / fitted);
    }
  }

  return expectedVals;
}

double calculatePoisson(const FunctionValues &vals) {
  double expectedVal = 0.0;
  for (size_t i = 0; i < vals.size(); i++) {
    const double fitted = vals.getCalculated(i);
    const double binCounts = vals.getFitData(i);

    // Formula is 2((y - count) + count * (log(count) - log(y)))
    // where y is the fitted men count rate
    double contribution = fitted - binCounts;
    contribution += binCounts * (std::log(binCounts) - std::log(fitted));
    expectedVal += 2 * (contribution);
  }

  return expectedVal;
}

FunctionDomain_sptr getFakeDomain(int startY, int endY) {
  // Ensure that the steps are not subdivided to keep testing simple
  // by making 1 step for each value including the starting val
  const int numSteps = endY - startY + 1;
  return boost::make_shared<FunctionDomain1DVector>(startY, endY, numSteps);
}

FunctionValues_sptr getFakeValues(const std::vector<double> &nValues,
                                  const FunctionDomain &domain) {
  // The number of fake values must match the domain points for the test to be
  // sane
  assert(domain.size() == nValues.size() &&
         "The number of points must match domain size");
  auto funcValues = boost::make_shared<FunctionValues>(domain);

  for (size_t i = 0; i < nValues.size(); i++) {
    funcValues->setFitData(i, nValues[i]);
  }

  return funcValues;
}

boost::shared_ptr<UserFunction> getFakeFunction() {
  auto func = boost::make_shared<UserFunction>();
  // By using x as our custom fitting formula we effectively map x->y
  // within the CostFuncPoisson method
  func->setAttributeValue("Formula", "x");
  return func;
}

} // namespace

class CostFuncPoissonTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CostFuncPoissonTest *createSuite() {
    return new CostFuncPoissonTest();
  }
  static void destroySuite(CostFuncPoissonTest *suite) { delete suite; }

  void test_y_at_0_returns_inf() {
    FunctionDomain_sptr domainAtZero = getFakeDomain(0, 2);
    FunctionValues_sptr vals = getFakeValues({1, 0, 1}, *domainAtZero);

    CostFuncPoisson atZero;
    atZero.setFittingFunction(getFakeFunction(), domainAtZero, vals);

    atZero.addVal(domainAtZero, vals);
    TS_ASSERT(std::isinf(atZero.val()));
  }

  void test_y_below_0_returns_inf() {
    FunctionDomain_sptr domainBelowZero = getFakeDomain(-1, 1);
    FunctionValues_sptr vals = getFakeValues({1, -1, 1}, *domainBelowZero);

    CostFuncPoisson belowZero;
    belowZero.setFittingFunction(getFakeFunction(), domainBelowZero, vals);

    belowZero.addVal(domainBelowZero, vals);
    TS_ASSERT(std::isinf(belowZero.val()));
  }

  void test_y_with_no_bin_contents() {
    // When y > 0 and the bin contents (N) = 0 it should return 2*y for cost
    auto mockFunction = getFakeFunction();
    FunctionDomain_sptr domain = getFakeDomain(1, 3);

    const std::vector<int> differenceVals{1, 2, 3};

    // Set N = 0 for no bin contents
    FunctionValues_sptr vals = getFakeValues({0, 0, 0}, *domain);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(getFakeFunction(), domain, vals);

    testInstance.addVal(domain, vals);

    const int sumOfValues =
        std::accumulate(differenceVals.begin(), differenceVals.end(), 0);
    TS_ASSERT_EQUALS(testInstance.val(), sumOfValues * 2)
  }

  void test_y_with_simple_bin_contents() {
    auto mockFunction = getFakeFunction();
    FunctionDomain_sptr domain = getFakeDomain(1, 3);
    FunctionValues_sptr vals = getFakeValues({1, 1, 1}, *domain);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(getFakeFunction(), domain, vals);

    testInstance.addVal(domain, vals);

    const double expected = calculatePoisson(*vals);

    TS_ASSERT_EQUALS(testInstance.val(), expected)
  }

  void test_y_with_bin_contents() {
    // Run test again with different values to check it works in both cases
    auto mockFunction = getFakeFunction();
    FunctionDomain_sptr domain = getFakeDomain(6, 8);
    FunctionValues_sptr vals = getFakeValues({10, 2, 1.5}, *domain);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(getFakeFunction(), domain, vals);

    testInstance.addVal(domain, vals);
    const double expected = calculatePoisson(*vals);

    TS_ASSERT_EQUALS(testInstance.val(), expected)
  }

  void test_y_below_cutoff() {
    // Run test again with different values to check it works in both cases
    auto mockFunction = getFakeFunction();

    const size_t numPoints = 10;

    auto domain = boost::make_shared<FunctionDomain1DVector>(
        (cutOffPoint / numPoints), cutOffPoint, numPoints);

    std::vector<double> fakeVals(numPoints);
    std::iota(fakeVals.begin(), fakeVals.end(), 1);
    FunctionValues_sptr vals = getFakeValues(fakeVals, *domain);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(getFakeFunction(), domain, vals);

    testInstance.addVal(domain, vals);

    double expectedVal = 0.0;
    for (size_t i = 0; i < vals->size(); i++) {
      const double fitted = vals->getCalculated(i);

      double contribution = (cutOffPoint - fitted) / fitted;
      expectedVal += 2 * (contribution);
    }

    TS_ASSERT_EQUALS(testInstance.val(), expectedVal)
  }

  void test_deriv_no_params() {
    FunctionDomain_sptr domain = getFakeDomain(1, 3);
    FunctionValues_sptr vals = getFakeValues({1, 2, 3}, *domain);

    auto mockFunction = boost::make_shared<UserFunction>();
    mockFunction->setAttributeValue("Formula", "x");

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(mockFunction, domain, vals);

    testInstance.addValDerivHessian(mockFunction, domain, vals);

    const auto expectedDet = calculateDeterminant(*vals, 0);
    const auto returnedDet = testInstance.getDeriv();
    for (size_t i = 0; i < expectedDet.size(); i++) {
      TS_ASSERT_EQUALS(returnedDet[i], expectedDet[i]);
    }
  }

  void test_deriv_below_cutoff() {
    // Below 0 regardless of active parameters the resulting derivative should
    // be inf
    auto domain = getFakeDomain(0, 1);

    FunctionValues_sptr vals = getFakeValues({1, 2}, *domain);

    auto mockFunction = boost::make_shared<UserFunction>();
    mockFunction->setAttributeValue("Formula", "x + a + b");
    mockFunction->setParameter("a", 0);
    mockFunction->setParameter("b", 0);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(mockFunction, domain, vals);

    testInstance.addValDerivHessian(mockFunction, domain, vals);

    TS_ASSERT(std::isinf(testInstance.getDeriv()[0]));
  }

  void test_deriv_with_params() {
    FunctionDomain_sptr domain = getFakeDomain(6, 8);
    FunctionValues_sptr vals = getFakeValues({10, 2, 1.5}, *domain);

    auto mockFunction = boost::make_shared<UserFunction>();
    mockFunction->setAttributeValue("Formula", "x + a + b");
    mockFunction->setParameter("a", 1);
    mockFunction->setParameter("b", 1);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(mockFunction, domain, vals);

    testInstance.addValDerivHessian(mockFunction, domain, vals);

    const auto expectedDet = calculateDeterminant(*vals, 0);
    const auto returnedDet = testInstance.getDeriv();
    for (size_t i = 0; i < expectedDet.size(); i++) {
      TS_ASSERT_EQUALS(returnedDet[i], expectedDet[i]);
    }
  }

  void test_hessian_with_params() {
    FunctionDomain_sptr domain = getFakeDomain(1, 3);
    FunctionValues_sptr vals = getFakeValues({1, 2, 3}, *domain);

    auto mockFunction = boost::make_shared<UserFunction>();
    mockFunction->setAttributeValue("Formula", "x + a + b");
    mockFunction->setParameter("a", 1);
    mockFunction->setParameter("b", 100);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(mockFunction, domain, vals);
    testInstance.addValDerivHessian(mockFunction, domain, vals);

    const auto returnedHessian = testInstance.getHessian();
    const auto firstRow = returnedHessian.copyRow(0);

    const std::vector<double> expectedVals{5.618e-4, 5.62e-4};

    for (size_t i = 0; i < expectedVals.size(); i++) {
      TS_ASSERT_DELTA(firstRow[i], expectedVals[i], 1e-7);
    }
  }

  void test_hessian_below_cutoff() {
    FunctionDomain_sptr domain = getFakeDomain(-1, 1);
    FunctionValues_sptr vals = getFakeValues({1, 2, 3}, *domain);

    auto mockFunction = boost::make_shared<UserFunction>();
    mockFunction->setAttributeValue("Formula", "x + a + b");
    mockFunction->setParameter("a", 0);
    mockFunction->setParameter("b", 0);

    CostFuncPoisson testInstance;
    testInstance.setFittingFunction(mockFunction, domain, vals);
    testInstance.addValDerivHessian(mockFunction, domain, vals);

    const auto returnedHessian = testInstance.getHessian();
    const auto firstRow = returnedHessian.copyRow(0);

    for (size_t i = 0; i < firstRow.size(); i++) {
      TS_ASSERT(std::isinf(firstRow[i]));
    }
  }
};

#endif /* MANTID_CURVEFITTING_COSTFUNCPOISSONTEST_H_ */