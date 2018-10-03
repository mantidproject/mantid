// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef GAUSSIANTEST_H_
#define GAUSSIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Constraints/BoundaryConstraint.h"
#include "MantidCurveFitting/CostFunctions/CostFuncLeastSquares.h"
#include "MantidCurveFitting/FuncMinimizers/LevenbergMarquardtMDMinimizer.h"
#include "MantidCurveFitting/Functions/Gaussian.h"
#include "MantidCurveFitting/Functions/LinearBackground.h"
#include "MantidCurveFitting/Functions/UserFunction.h"

using namespace Mantid;
using namespace Mantid::Kernel;
using namespace Mantid::API;
using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::CurveFitting::CostFunctions;
using namespace Mantid::CurveFitting::Constraints;

// Algorithm to force Gaussian1D to be run by simplex algorithm
class SimplexGaussian : public Gaussian {
public:
  ~SimplexGaussian() override {}
  std::string name() const override { return "SimplexGaussian"; }

protected:
  void functionDerivMW(Jacobian *out, const double *xValues,
                       const size_t nData) {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
    throw Exception::NotImplementedError("No derivative function provided");
  }
};

DECLARE_FUNCTION(SimplexGaussian)

class GaussianTest : public CxxTest::TestSuite {
public:
  void test_category() {
    Gaussian fn;
    TS_ASSERT_EQUALS(fn.category(), "Peak");
  }

  void test_with_Levenberg_Marquardt() {
    API::FunctionDomain1D_sptr domain(
        new API::FunctionDomain1DVector(79292.4, 79603.6, 41));
    API::FunctionValues mockData(*domain);
    UserFunction dataMaker;
    dataMaker.setAttributeValue("Formula", "h*exp(-((x-c)/s)^2)");
    dataMaker.setParameter("h", 232.11);
    dataMaker.setParameter("c", 79430.1);
    dataMaker.setParameter("s", 26.14);
    dataMaker.function(*domain, mockData);

    API::FunctionValues_sptr values(new API::FunctionValues(*domain));
    values->setFitDataFromCalculated(mockData);
    values->setFitWeights(1.0);

    // set up Gaussian fitting function
    boost::shared_ptr<Gaussian> fn = boost::make_shared<Gaussian>();
    fn->initialize();
    fn->setParameter("PeakCentre", 79440.0);
    fn->setParameter("Height", 200.0);
    fn->setParameter("Sigma", 30.0);

    boost::shared_ptr<CostFuncLeastSquares> costFun =
        boost::make_shared<CostFuncLeastSquares>();
    costFun->setFittingFunction(fn, domain, values);

    FuncMinimisers::LevenbergMarquardtMDMinimizer s;
    s.initialize(costFun);
    TS_ASSERT(s.minimize());

    API::IFunction_sptr res = costFun->getFittingFunction();
  }

  void testIntensity() {
    boost::shared_ptr<Gaussian> fn = boost::make_shared<Gaussian>();
    fn->initialize();
    fn->setHeight(2.0);
    fn->setFwhm(0.125);
    fn->setCentre(-200.0);

    // Area under a gaussian is height * sigma * sqrt(2 * pi)
    TS_ASSERT_DELTA(fn->intensity(), 0.26611675485780654483, 1e-10);
  }

  void testSetIntensity() {
    boost::shared_ptr<Gaussian> fn = boost::make_shared<Gaussian>();
    fn->initialize();
    fn->setHeight(2.0);
    fn->setFwhm(0.125);
    fn->setCentre(-200.0);

    TS_ASSERT_THROWS_NOTHING(fn->setIntensity(0.5));

    TS_ASSERT_DELTA(fn->intensity(), 0.5, 1e-10);

    // FWHM does not change
    TS_ASSERT_EQUALS(fn->fwhm(), 0.125);

    // Height changes
    TS_ASSERT_DELTA(fn->height(), 3.75774911479860533509, 1e-10);
  }

  void testSetIntensityDefault() {
    boost::shared_ptr<Gaussian> fn = boost::make_shared<Gaussian>();
    fn->initialize();

    TS_ASSERT_EQUALS(fn->intensity(), 0.0);

    TS_ASSERT_THROWS_NOTHING(fn->setIntensity(20.0));
    TS_ASSERT_EQUALS(fn->intensity(), 20.0);

    // Now, fwhm is not zero
    fn->setFwhm(0.02);

    TS_ASSERT_THROWS_NOTHING(fn->setIntensity(20.0));
    TS_ASSERT_DELTA(fn->intensity(), 20.0, 1e-10);
  }

  void testGetCentreParameterName() {
    boost::shared_ptr<Gaussian> fn = boost::make_shared<Gaussian>();
    fn->initialize();

    TS_ASSERT_THROWS_NOTHING(fn->getCentreParameterName());
    TS_ASSERT_EQUALS(fn->getCentreParameterName(), "PeakCentre");
  }

  void test_fixing() {
    Gaussian fn;
    fn.initialize();

    fn.fixCentre();
    auto i = fn.parameterIndex("PeakCentre");
    TS_ASSERT(fn.isFixed(i));
    fn.unfixCentre();
    TS_ASSERT(!fn.isFixed(i));

    fn.setParameter("Height", 1.0);
    auto intensity = fn.intensity();
    fn.fixIntensity();
    fn.setParameter("Sigma", 2.0);
    fn.applyTies();
    TS_ASSERT_DELTA(fn.intensity(), intensity, 1e-6);
    TS_ASSERT_DELTA(fn.getParameter("Height"), 0.199471, 1e-6);

    fn.setParameter("Sigma", 3.0);
    fn.applyTies();
    TS_ASSERT_DELTA(fn.intensity(), intensity, 1e-6);
    TS_ASSERT_DELTA(fn.getParameter("Height"), 0.132981, 1e-6);

    fn.setParameter("Sigma", 0.01);
    fn.applyTies();
    TS_ASSERT_DELTA(fn.intensity(), intensity, 1e-6);

    fn.setParameter("Sigma", 1.0);
    fn.applyTies();
    TS_ASSERT_DELTA(fn.intensity(), intensity, 1e-6);
    TS_ASSERT_DELTA(fn.getParameter("Height"), 0.398942, 1e-6);
  }
};

#endif /*GAUSSIANTEST_H_*/