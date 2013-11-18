#ifndef LORENTZIANTEST_H_
#define LORENTZIANTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidCurveFitting/Lorentzian.h"
#include "MantidCurveFitting/Jacobian.h"

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
    func->setParameter("HWHM", 5);
    func->setParameter("PeakCentre", 2.0);
    return func;
  }

};

#endif /*LORENTZIANTEST_H_*/
