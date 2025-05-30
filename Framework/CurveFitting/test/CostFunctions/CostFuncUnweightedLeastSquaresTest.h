// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/CostFunctions/CostFuncUnweightedLeastSquares.h"

#include <memory>

using Mantid::CurveFitting::CostFunctions::CostFuncUnweightedLeastSquares;
using namespace Mantid::API;

class CostFuncUnweightedLeastSquaresTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CostFuncUnweightedLeastSquaresTest *createSuite() { return new CostFuncUnweightedLeastSquaresTest(); }
  static void destroySuite(CostFuncUnweightedLeastSquaresTest *suite) { delete suite; }

  void testGetFitWeights() {
    /* The test makes sure that the returned weights are always 1.0,
     * except when the original weight was 0.
     */
    FunctionDomain1DVector d1d(std::vector<double>(20, 1.0));
    FunctionValues_sptr values = std::make_shared<FunctionValues>(d1d);

    for (size_t i = 0; i < values->size(); ++i) {
      values->setFitWeight(i, static_cast<double>(i));
    }

    TestableCostFuncUnweightedLeastSquares uwls;

    std::vector<double> weights = uwls.getFitWeights(values);

    TS_ASSERT_EQUALS(weights.size(), values->size());
    TS_ASSERT_EQUALS(weights.front(), 0)
    for (size_t i = 1; i < weights.size(); ++i) {
      TS_ASSERT_EQUALS(weights[i], 1.0);
    }
  }

  void testGetResidualVariance() {
    /* Make sure that the calculated residuals variance is correct. The
     * test uses dummy values for which the sum of residuals is known.
     */
    FunctionDomain1DVector d1d(std::vector<double>(10, 1.0));
    FunctionValues_sptr values = std::make_shared<FunctionValues>(d1d);

    // Data generated with numpy.random.normal(loc=2.0, scale=0.25, size=10)
    double obsValues[10] = {1.9651563160778176, 1.9618188576389295, 1.9565961107376706, 2.0049055113975252,
                            2.0747505383068865, 2.0666404554638578, 1.7854026688169637, 2.266075963037971,
                            1.8656602424955859, 1.8132221813342393};

    for (size_t i = 0; i < 10; ++i) {
      values->setCalculated(2.0);
      values->setFitData(i, obsValues[i]);
      values->setFitWeights(0.0);
    }

    // Function has 1 parameter, so degrees of freedom = 9
    IFunction_sptr fn = FunctionFactory::Instance().createFunction("FlatBackground");
    FunctionDomain_sptr domain = std::make_shared<FunctionDomain1DVector>(d1d);

    TestableCostFuncUnweightedLeastSquares uwls;
    uwls.setFittingFunction(fn, domain, values);

    double variance = uwls.getResidualVariance();
    TS_ASSERT_DELTA(variance, 0.0204877770575, 1e-13);
  }

  void testUpdatedFitWeights() {
    FunctionDomain1DVector d1d(std::vector<double>(10, 1.0));
    FunctionValues_sptr values = std::make_shared<FunctionValues>(d1d);
    for (size_t i = 0; i < 10; ++i) {
      values->setFitWeights(5.0); // These weights would be reset to 1 when fitting func is set
    }

    IFunction_sptr fn = FunctionFactory::Instance().createFunction("FlatBackground");
    FunctionDomain_sptr domain = std::make_shared<FunctionDomain1DVector>(d1d);

    TestableCostFuncUnweightedLeastSquares uwls;
    uwls.setFittingFunction(fn, domain, values);
    auto updated_values = uwls.getValues();
    for (size_t i = 0; i < updated_values->size(); i++) {
      TS_ASSERT_EQUALS(updated_values->getFitWeight(i), 1.0);
    }
  }

  void testUpdatedFitWeights_with_ignore_invalid_values() {
    FunctionDomain1DVector d1d(std::vector<double>(10, 1.0));
    FunctionValues_sptr values = std::make_shared<FunctionValues>(d1d);
    for (size_t i = 0; i < 10; ++i) {
      values->setFitWeights(0.0); // These weights would be reset
    }

    IFunction_sptr fn = FunctionFactory::Instance().createFunction("FlatBackground");
    FunctionDomain_sptr domain = std::make_shared<FunctionDomain1DVector>(d1d);

    TestableCostFuncUnweightedLeastSquares uwls;
    uwls.setFittingFunction(fn, domain, values);
    uwls.setIgnoreInvalidData(true);
    auto updated_values = uwls.getValues();
    for (size_t i = 0; i < updated_values->size(); i++) {
      TS_ASSERT_EQUALS(updated_values->getFitWeight(i), 1.0);
    }
  }

private:
  class TestableCostFuncUnweightedLeastSquares : public CostFuncUnweightedLeastSquares {
    friend class CostFuncUnweightedLeastSquaresTest;
  };
};
