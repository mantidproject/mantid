#ifndef BACKTOBACKEXPONENTIALTEST_H_
#define BACKTOBACKEXPONENTIALTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/BackToBackExponential.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <cmath>

using Mantid::CurveFitting::BackToBackExponential;

namespace 
{
  /**
   * A simple test function: two exponentials back-to-back.
   */
  class B2B : public Mantid::API::IFunction1D, public Mantid::API::ParamFunction
  {
  public:
    /// Default constructor.
    B2B():Mantid::API::IFunction1D(), Mantid::API::ParamFunction()
    {
      declareParameter("A",1.0);
      declareParameter("B",2.0);
    }
    /// overwrite IFunction base class methods
    std::string name()const{return "B2B";}
    virtual const std::string category() const { return "Peak";}
    virtual void function1D(double* out, const double* xValues, const size_t nData)const
    {
      const double a = getParameter(0);
      const double b = getParameter(1);
      for(size_t i = 0; i < nData; ++i)
      {
        double x = xValues[i];
        out[i] = x <= 0.0 ? exp( a * x ) : exp( - b * x );
      }
    }
    // return the integral of this function in (-oo, oo)
    double integral() const
    {
      const double a = getParameter(0);
      const double b = getParameter(1);
      return (a + b) / (a * b);
    }
  };
}

class BackToBackExponentialTest : public CxxTest::TestSuite
{
public:

  void test_test_function_B2B()
  {
    // define 1d domain of 30 points in interval [-6,3]
    Mantid::API::FunctionDomain1DVector x(-6,3,30);
    Mantid::API::FunctionValues y(x);
    B2B b2b;
    b2b.function(x,y);
    for(size_t i = 0; i < x.size(); ++i)
    {
      if ( x[i] <= 0 )
      {
        TS_ASSERT_DELTA(y[i], exp( x[i] ), 1e-15);
      }
      else
      {
        TS_ASSERT_DELTA(y[i], exp(-2*x[i] ), 1e-15);
      }
    }
  }

  void testForCategories()
  {
    BackToBackExponential forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Peak" );
  }

  // test that parameters exist and can be set and read
  void test_parameters()
  {
    BackToBackExponential b2bExp; 
    TS_ASSERT_EQUALS( b2bExp.nParams(), 0 );
    b2bExp.initialize();
    TS_ASSERT_EQUALS( b2bExp.nParams(), 5 );
    b2bExp.setParameter("I", 123.45);
    TS_ASSERT_EQUALS( b2bExp.getParameter("I"), 123.45 );
    b2bExp.setParameter("A", 23.45);
    TS_ASSERT_EQUALS( b2bExp.getParameter("A"), 23.45 );
    b2bExp.setParameter("B", 3.45);
    TS_ASSERT_EQUALS( b2bExp.getParameter("B"), 3.45 );
    b2bExp.setParameter("X0", .45);
    TS_ASSERT_EQUALS( b2bExp.getParameter("X0"), .45 );
    b2bExp.setParameter("S", 4.5);
    TS_ASSERT_EQUALS( b2bExp.getParameter("S"), 4.5 );
  }

  // test that parameter I equals integrated intensity of the peak
  void test_integrated_intensity()
  {
    BackToBackExponential b2bExp; 
    b2bExp.initialize();
    b2bExp.setParameter("I", 1.0);
    b2bExp.setParameter("A", 1.1);
    b2bExp.setParameter("B", 2.2);
    b2bExp.setParameter("X0",0.0);
    b2bExp.setParameter("S", 4.0);

    Mantid::API::FunctionDomain1DVector x(-20,20,100);
    Mantid::API::FunctionValues y(x);
    b2bExp.function(x,y);
    double sum = 0.0;
    for(size_t i = 0; i < x.size(); ++i) sum += y[i];
    sum *= x[1] - x[0];
    TS_ASSERT_DELTA(sum, 1.0, 0.00001);
    b2bExp.setParameter("I", 2.3);
    b2bExp.function(x,y);
    sum = 0.0;
    for(size_t i = 0; i < x.size(); ++i) sum += y[i];
    sum *= x[1] - x[0];
    TS_ASSERT_DELTA(sum, 2.3, 0.00001);
  }

  // test that when gaussian is narrow BackToBackExponential tends to B2B
  void test_narrow_gaussian()
  {
    const double a = 1.0;
    const double b = 2.0;
    const double I = 2.1;
    BackToBackExponential b2bExp; 
    b2bExp.initialize();
    b2bExp.setParameter("I", I);
    b2bExp.setParameter("A", a);
    b2bExp.setParameter("B", b);
    b2bExp.setParameter("X0",0.0);
    b2bExp.setParameter("S", 0.000001);// this makes gaussian narrow

    B2B b2b;
    b2b.setParameter("A", a);
    b2b.setParameter("B", b);
    const double b2bNorm = b2b.integral();

    // define 1d domain of 30 points in interval [-6,3]
    Mantid::API::FunctionDomain1DVector x(-6,3,30);
    Mantid::API::FunctionValues y1(x), y2(x);

    b2bExp.function(x,y1);
    b2b.function(x,y2);

    for(size_t i = 0; i < x.size(); ++i)
    {
      TS_ASSERT_DELTA( y1[i], I*y2[i]/b2bNorm, 1e-10 );
    }
  }

  // test that when gaussian is wide BackToBackExponential tends to Gaussian
  void test_wide_gaussian()
  {
    const double s = 4.0;
    const double I = 2.1;
    BackToBackExponential b2bExp; 
    b2bExp.initialize();
    b2bExp.setParameter("I", I);
    b2bExp.setParameter("A", 6.0);// large A and B make
    b2bExp.setParameter("B", 6.0);// the exponentials narrow
    b2bExp.setParameter("X0",0.0);
    b2bExp.setParameter("S", s);

    // define 1d domain of 30 points in interval [-6,3]
    Mantid::API::FunctionDomain1DVector x(-10,10,30);
    Mantid::API::FunctionValues y(x);

    b2bExp.function(x,y);

    for(size_t i = 0; i < x.size(); ++i)
    {
      double arg = x[i]/s;
      arg *= arg;
      double ex = I*exp(-arg/2)/sqrt(2*M_PI)/s;
      TS_ASSERT_DELTA( y[i] / ex, 1.0, 0.01 );
    }
  }

  void testIntensity()
  {
      const double s = 4.0;
      const double I = 2.1;
      BackToBackExponential b2bExp;
      b2bExp.initialize();
      b2bExp.setParameter("I", I);
      b2bExp.setParameter("A", 6.0);// large A and B make
      b2bExp.setParameter("B", 6.0);// the exponentials narrow
      b2bExp.setParameter("X0",0.0);
      b2bExp.setParameter("S", s);

      TS_ASSERT_EQUALS(b2bExp.intensity(), 2.1);
      TS_ASSERT_THROWS_NOTHING(b2bExp.setIntensity(3.0));

      TS_ASSERT_EQUALS(b2bExp.intensity(), 3.0);
      TS_ASSERT_EQUALS(b2bExp.getParameter("I"), 3.0);
  }

  void test_width()
  {
    BackToBackExponential b2b; 
    b2b.initialize();
    //b2b.setParameter("I", 10);
    //b2b.setParameter("A", 200.0);// large A and B make
    //b2b.setParameter("B", 100.0);// the exponentials narrow
    //b2b.setParameter("X0",0.0);
    //b2b.setParameter("S", .00001);

    std::cerr << "Test width " << b2b.centre() << ' ' << b2b.height() << ' ' << b2b.fwhm() << std::endl;

    double vals[] = {1,2,3,4,5};
    for(size_t i = 0; i < sizeof(vals)/sizeof(double); ++i)
    {
      b2b.setParameter("S", vals[i]);
      std::cerr << "S " << vals[i] << ' ' << b2b.fwhm() << std::endl;
    }

  }

};

#endif /*BACKTOBACKEXPONENTIALTEST_H_*/
