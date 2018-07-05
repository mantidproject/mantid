#ifndef MANTID_CURVEFITTING_PRODUCTQUADRATICEXPTEST_H_
#define MANTID_CURVEFITTING_PRODUCTQUADRATICEXPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionValues.h"
#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidCurveFitting/Functions/ProductFunction.h"
#include "MantidCurveFitting/Functions/ProductQuadraticExp.h"
#include "MantidCurveFitting/Functions/Quadratic.h"
#include "MantidCurveFitting/Jacobian.h"
#include <algorithm>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

using namespace Mantid::CurveFitting;
using namespace Mantid::CurveFitting::Functions;
using namespace Mantid::API;

class ProductQuadraticExpTest : public CxxTest::TestSuite {

private:
  /// Helper type to generate number for a std::generate call.
  class LinearIncrementingAssignment {
  private:
    double m_current;
    const double m_step;

  public:
    LinearIncrementingAssignment(double min, double step)
        : m_current(min), m_step(step) {}
    double operator()() {
      double temp = m_current;
      m_current += m_step;
      return temp;
    }
  };

  /** Helper method
With the input arguments
1) Creates the target ProductQuadraticFunction.
2) Creates and equavalent Product function using other Fit function framework
types.
3) Manually calculates the expected output given that the underlying equation is
so simple.
4) For each point on the domain, compares the outputs of (1) (2) and (3) above
to check that the results are equal.
*/
  void do_test_function_calculation(const double &A0, const double &A1,
                                    const double &A2, const double &Height,
                                    const double &Lifetime) {
    // Create the Product linear exponential function
    ProductQuadraticExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("A2", A2);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    // Create the equivalent Product Function
    IFunction_sptr quadraticFunction = boost::make_shared<Quadratic>();
    quadraticFunction->initialize();
    quadraticFunction->setParameter("A0", A0);
    quadraticFunction->setParameter("A1", A1);
    quadraticFunction->setParameter("A2", A2);
    IFunction_sptr expFunction = boost::make_shared<ExpDecay>();
    expFunction->initialize();
    expFunction->setParameter("Height", Height);
    expFunction->setParameter("Lifetime", Lifetime);
    ProductFunction benchmark;
    benchmark.initialize();
    benchmark.addFunction(quadraticFunction);
    benchmark.addFunction(expFunction);

    const size_t nResults = 10;
    using VecDouble = std::vector<double>;
    VecDouble xValues(nResults);
    std::generate(xValues.begin(), xValues.end(),
                  LinearIncrementingAssignment(0, 0.0001));

    FunctionDomain1DVector domain(xValues);
    FunctionValues valuesBenchmark(domain);
    FunctionValues valuesLinExpDecay(domain);
    // Peform function evaluations.
    benchmark.function(domain, valuesBenchmark);
    func.function(domain, valuesLinExpDecay);

    for (size_t i = 0; i < nResults; ++i) {
      double x = xValues[i];
      // Mimic workings of ProductLinearExp function to product comparison
      // output.
      double expected =
          ((A2 * x * x) + (A1 * x) + A0) * Height * std::exp(-x / Lifetime);
      TS_ASSERT_DELTA(expected, valuesLinExpDecay[i], 0.0001);

      // Verify that the output is also the same for the Linear algorithm.
      TS_ASSERT_DELTA(valuesBenchmark[i], valuesLinExpDecay[i], 0.0001);
    }
  }

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProductQuadraticExpTest *createSuite() {
    return new ProductQuadraticExpTest();
  }
  static void destroySuite(ProductQuadraticExpTest *suite) { delete suite; }

  void test_name() {
    ProductQuadraticExp func;
    TS_ASSERT_EQUALS("ProductQuadraticExp", func.name());
  }

  void test_category() {
    ProductQuadraticExp func;
    TS_ASSERT_EQUALS("Calibrate", func.category());
  }

  void test_registered_with_factory() {
    IFunction_sptr func =
        FunctionFactory::Instance().createFunction("ProductQuadraticExp");
    TS_ASSERT(func != nullptr);
    TS_ASSERT_EQUALS(func->name(), "ProductQuadraticExp");
    TS_ASSERT(boost::dynamic_pointer_cast<ProductQuadraticExp>(func) !=
              nullptr);
  }

  void test_set_parameters() {
    const double A0 = 1;
    const double A1 = 2;
    const double A2 = 3;
    const double Height = 4;
    const double Lifetime = 0.1;

    ProductQuadraticExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("A2", A2);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    TS_ASSERT_EQUALS(A0, func.getParameter("A0"));
    TS_ASSERT_EQUALS(A1, func.getParameter("A1"));
    TS_ASSERT_EQUALS(A2, func.getParameter("A2"));
    TS_ASSERT_EQUALS(Height, func.getParameter("Height"));
    TS_ASSERT_EQUALS(Lifetime, func.getParameter("Lifetime"));
  }

  void test_execution_with_exp_components_unity() {
    // A1 is set to zero, so the ProductLinearExp function should just reduce to
    // an exp decay function.
    const double A0 = 1;
    const double A1 = 0;
    const double A2 = 0;
    const double Height = 2;
    const double Lifetime = 0.1;

    ProductQuadraticExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("A2", A2);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    ExpDecay benchmark;
    benchmark.setParameter("Height", Height);
    benchmark.setParameter("Lifetime", Lifetime);

    const size_t nResults = 10;
    using VecDouble = std::vector<double>;
    VecDouble xValues(nResults);
    std::generate(xValues.begin(), xValues.end(),
                  LinearIncrementingAssignment(0, 0.1));

    FunctionDomain1DVector domain(xValues);
    FunctionValues valuesExpDecay(domain);
    FunctionValues valuesLinExpDecay(domain);
    benchmark.function(domain, valuesExpDecay);
    func.function(domain, valuesLinExpDecay);

    for (size_t i = 0; i < nResults; ++i) {
      double x = xValues[i];
      // Mimic workings of ProductLinearExp function to product comparison
      // output.
      double expected = ((A1 * x) + A0) * Height * std::exp(-x / Lifetime);
      TS_ASSERT_DELTA(expected, valuesLinExpDecay[i], 0.0001);
      // As a complete check, verify that the output is also the same for the
      // ExpDecay algorithm.
      TS_ASSERT_DELTA(valuesExpDecay[i], valuesLinExpDecay[i], 0.0001);
    }
  }

  void test_calculate_derivative_throws_nothing() {
    const size_t nResults = 10;
    using VecDouble = std::vector<double>;
    VecDouble xValues(nResults);
    std::generate(xValues.begin(), xValues.end(),
                  LinearIncrementingAssignment(0, 0.1));
    FunctionDomain1DVector domain(xValues);

    Mantid::CurveFitting::Jacobian jacobian(nResults, 5);

    ProductQuadraticExp func;
    TS_ASSERT_THROWS_NOTHING(func.functionDeriv(domain, jacobian));
  }

  void test_with_low_contribution_from_expdecay() {
    // Setup the test to for low contribution from exponential component.
    const double A0 = 0.2;
    const double A1 = 0.1;
    const double A2 = 0.05;
    const double Height = 1;
    const double Lifetime = 100;

    do_test_function_calculation(A0, A1, A2, Height, Lifetime);
  }

  void test_with_high_contribution_from_expdecay() {
    // Setup the test to for high contribution from exponential component.
    const double A0 = 0.2;
    const double A1 = 0.1;
    const double A2 = 0.05;
    const double Height = 1;
    const double Lifetime = 0.01;

    do_test_function_calculation(A0, A1, A2, Height, Lifetime);
  }
};

#endif /* MANTID_CURVEFITTING_PRODUCTQUADRATICEXPTEST_H_ */
