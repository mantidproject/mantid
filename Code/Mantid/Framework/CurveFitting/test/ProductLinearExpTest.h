#ifndef MANTID_CURVEFITTING_PRODUCTLINEAREXPTEST_H_
#define MANTID_CURVEFITTING_PRODUCTLINEAREXPTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/ProductLinearExp.h"
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/ProductFunction.h"
#include "MantidAPI/FunctionDomain1D.h" 
#include "MantidAPI/FunctionValues.h" 
#include <algorithm>

using namespace Mantid::CurveFitting;
using namespace Mantid::API;

class ProductLinearExpTest : public CxxTest::TestSuite
{
private:

  /// Helper type to generate number for a std::generate call.
  class LinearIncrementingAssignment
  {
  private:
    const double m_step;
    double m_current;
  public:
    LinearIncrementingAssignment(double min, double step) : m_current(min), m_step(step)
    {
    }
    double operator() () 
    {
      double temp = m_current;
      m_current += m_step;
      return temp;
    }
  };

public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static ProductLinearExpTest *createSuite() { return new ProductLinearExpTest(); }
  static void destroySuite( ProductLinearExpTest *suite ) { delete suite; }

  void test_name()
  {
    ProductLinearExp func;
    TS_ASSERT_EQUALS("ProductLinearExp", func.name());
  }

  void test_catagory()
  {
    ProductLinearExp func;
    TS_ASSERT_EQUALS("Calibrate", func.category());
  }

  void test_set_parameters()
  {
    const double A0 = 1;
    const double A1 = 2;
    const double Height = 3;
    const double Lifetime = 0.1;
    
    ProductLinearExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    TS_ASSERT_EQUALS(A0, func.getParameter("A0"));
    TS_ASSERT_EQUALS(A1, func.getParameter("A1"));
    TS_ASSERT_EQUALS(Height, func.getParameter("Height"));
    TS_ASSERT_EQUALS(Lifetime, func.getParameter("Lifetime"));
  }

  void test_execution_with_exp_components_unity()
  {
    // A1 is set to zero, so the ProductLinearExp function should just reduce to an exp decay function.
    const double A0 = 1;
    const double A1 = 0;
    const double Height = 2;
    const double Lifetime = 0.1;
    
    ProductLinearExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    ExpDecay benchmark;
    benchmark.setParameter("Height", Height);
    benchmark.setParameter("Lifetime", Lifetime);

    const size_t nResults = 10;
    typedef std::vector<double> VecDouble;
    VecDouble xValues(nResults);
    std::generate(xValues.begin(), xValues.end(), LinearIncrementingAssignment(0, 0.1));

    FunctionDomain1DVector domain(xValues);
    FunctionValues valuesExpDecay(domain);
    FunctionValues valuesLinExpDecay(domain);
    benchmark.function(domain,valuesExpDecay);
    func.function(domain, valuesLinExpDecay);

    for(size_t i = 0; i < nResults; ++i)
    {
      double x = xValues[i];
      // Mimic workings of ProductLinearExp function to product comparison output.
      double expected = ((A1 * x) + A0)* Height * std::exp(-x/Lifetime); 
      TS_ASSERT_EQUALS(expected, valuesLinExpDecay[i]);
      // As a complete check, verify that the output is also the same for the ExpDecay algorithm.
      TS_ASSERT_EQUALS(valuesExpDecay[i], valuesLinExpDecay[i]);
    }
  }

  void test_execution_with_equivalent_composite()
  {
    
    const double A0 = 2;
    const double A1 = 1;
    const double Height = 1;
    const double Lifetime = 0;
    
    // Create the Product linear exponential function
    ProductLinearExp func;
    func.setParameter("A0", A0);
    func.setParameter("A1", A1);
    func.setParameter("Height", Height);
    func.setParameter("Lifetime", Lifetime);

    // Create the equivalent Product Function
    IFunction_sptr linearFunction = boost::make_shared<LinearBackground>();
    linearFunction->initialize();
    linearFunction->setParameter("A0", A0);
    linearFunction->setParameter("A1", A1);
    IFunction_sptr expFunction =  boost::make_shared<ExpDecay>();
    expFunction->initialize();
    expFunction->setParameter("Height", Height);
    expFunction->setParameter("Lifetime", Lifetime);
    ProductFunction benchmark;
    benchmark.initialize();
    benchmark.addFunction(linearFunction);
    benchmark.addFunction(expFunction);

    const size_t nResults = 10;
    typedef std::vector<double> VecDouble;
    VecDouble xValues(nResults);
    std::generate(xValues.begin(), xValues.end(), LinearIncrementingAssignment(0, 0.1));
 
    FunctionDomain1DVector domain(xValues);
    FunctionValues valuesLinear(domain);
    FunctionValues valuesLinExpDecay(domain);
    benchmark.function(domain,valuesLinear);
    func.function(domain, valuesLinExpDecay);

    for(size_t i = 0; i < nResults; ++i)
    {
      double x = xValues[i];
      // Mimic workings of ProductLinearExp function to product comparison output.
      double expected = ((A1 * x) + A0)* Height * std::exp(-x/Lifetime); 
      TS_ASSERT_EQUALS(expected, valuesLinExpDecay[i]);
      // As a complete check, verify that the output is also the same for the Linear algorithm.
      //TS_ASSERT_EQUALS(valuesLinear[i], valuesLinExpDecay[i]);
    }
  }



};


#endif /* MANTID_CURVEFITTING_PRODUCTLINEAREXPTEST_H_ */