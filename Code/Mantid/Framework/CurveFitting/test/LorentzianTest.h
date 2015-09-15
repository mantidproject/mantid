#ifndef LORENTZIANTEST_H_
#define LORENTZIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Lorentzian.h"
#include "MantidCurveFitting/Jacobian.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <boost/make_shared.hpp>

class LorentzianTest : public CxxTest::TestSuite
{
public:

  void test_function_gives_expected_value_for_given_input()
  {
    auto func = createTestLorentzian();
    const size_t nData(1);
    std::vector<double> xValues(nData, 2.5);

    std::vector<double> calculatedValues(nData, 0);
    func->functionLocal(calculatedValues.data(), xValues.data(), nData);

    TS_ASSERT_DELTA(calculatedValues[0], 0.24485376, 1e-8);
  }

  void test_jacobian_gives_expected_values()
  {
    auto func = createTestLorentzian();
    const size_t nData(1);
    std::vector<double> xValues(nData, 2.5);

    Mantid::CurveFitting::Jacobian jacobian(nData, 3);
    func->functionDerivLocal(&jacobian, xValues.data(), nData);

    double dfda = jacobian.get(0,0);
    double dfdxo = jacobian.get(0,1);
    double dfdg = jacobian.get(0,2);

    TS_ASSERT_DELTA(dfda, 0.12242688, 1e-8);
    TS_ASSERT_DELTA(dfdxo, 0.03766981, 1e-8);
    TS_ASSERT_DELTA(dfdg, -0.04520377, 1e-8);
  }

  void test_categories()
  {
    Mantid::CurveFitting::Lorentzian forCat;
    const std::vector<std::string> categories = forCat.categories();
    TS_ASSERT( categories.size() == 1 );
    TS_ASSERT( categories[0] == "Peak" );
  }

  void test_FWHM()
  {
      double hwhm = 0.5;
      Mantid::CurveFitting::Lorentzian lor;
      lor.initialize();
      lor.setParameter("Amplitude",1.0);
      lor.setParameter("PeakCentre",0.0);
      lor.setParameter("FWHM",hwhm*2);

      Mantid::API::FunctionDomain1DVector x(0,hwhm,2);
      Mantid::API::FunctionValues y(x);
      lor.function(x,y);

      TS_ASSERT_DELTA( y[1]/y[0], 0.5, 1e-15 );
  }

  void test_height()
  {
    Mantid::CurveFitting::Lorentzian lor;
    lor.initialize();
    lor.setHeight(2.0);
    lor.setCentre(3.0);
    lor.setFwhm(1.0);

    std::vector<double> x(1, lor.centre());
    std::vector<double> y(1, 0.0);

    lor.function1D(y.data(), x.data(), 1);

    TS_ASSERT_EQUALS(y[0], lor.height());
  }

  void test_height_zero_width()
  {
    Mantid::CurveFitting::Lorentzian lor;
    lor.initialize();
    lor.setHeight(2.0);
    lor.setCentre(3.0);
    lor.setFwhm(0.0);

    std::vector<double> x(1, lor.centre());
    std::vector<double> y(1, 0.0);

    lor.function1D(y.data(), x.data(), 1);

    // height is saved inside lor
    TS_ASSERT_EQUALS(2.0, lor.height());
    // lor is 0.0 everywhere
    TS_ASSERT_EQUALS(y[0], 0.0);
  }

  void testIntensity()
  {
      Mantid::CurveFitting::Lorentzian lor;
      lor.initialize();
      lor.setHeight(2.0);
      lor.setCentre(3.0);
      lor.setFwhm(1.0);

      TS_ASSERT_DELTA(lor.intensity(), 1.873097930277787, 1e-10);
      TS_ASSERT_THROWS_NOTHING(lor.setIntensity(2.0));

      TS_ASSERT_DELTA(lor.intensity(), 2.0, 1e-10);
      TS_ASSERT_EQUALS(lor.fwhm(), 1.0);
  }

private:
  
  class TestableLorentzian : public Mantid::CurveFitting::Lorentzian
  {
  public:
    void functionLocal(double* out, const double* xValues, const size_t nData)const
    {
      Mantid::CurveFitting::Lorentzian::functionLocal(out,xValues,nData);
    }
    void functionDerivLocal(Mantid::API::Jacobian* out, const double* xValues, const size_t nData)
    {
      Mantid::CurveFitting::Lorentzian::functionDerivLocal(out,xValues,nData);
    }
  };

  boost::shared_ptr<TestableLorentzian> createTestLorentzian()
  {
    auto func = boost::make_shared<TestableLorentzian>();
    func->initialize();
    func->setParameter("Amplitude", 2.0);
    func->setParameter("FWHM", 5);
    func->setParameter("PeakCentre", 2.0);
    return func;
  }

};

#endif /*LORENTZIANTEST_H_*/
