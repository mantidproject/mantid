#ifndef IFUNCTION1DTEST_H_
#define IFUNCTION1DTEST_H_

#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/FunctionDomain1D.h"
#include "MantidAPI/FunctionValues.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

const double A = 1.1;
const double B = 2.2;

class IFunction1DTest_Function : public virtual IFunction1D,
                                 public virtual ParamFunction {
protected:
  std::string name() const override { return "IFunction1DTest_Function"; }
  void function1D(double *out, const double *xValues,
                  const size_t nData) const override {
    for (size_t i = 0; i < nData; ++i) {
      double x = xValues[i];
      out[i] = A * x + B;
    }
  }
  void functionDeriv1D(Jacobian *out, const double *xValues,
                       const size_t nData) override {
    for (size_t i = 0; i < nData; ++i) {
      double x = xValues[i];
      out->set(i, 0, x);
      out->set(i, 1, 1.0);
    }
  }
};

class IFunction1DTest_Jacobian : public Jacobian {
public:
  IFunction1DTest_Jacobian(size_t ny, size_t np) : m_np(np) {
    m_data.resize(ny * np);
  }
  void set(size_t iY, size_t iP, double value) override {
    m_data[iY * m_np + iP] = value;
  }
  double get(size_t iY, size_t iP) override { return m_data[iY * m_np + iP]; }
  void zero() override { m_data.assign(m_data.size(), 0.0); }

private:
  size_t m_np;
  std::vector<double> m_data;
};

class IFunction1DTest : public CxxTest::TestSuite {
public:
  void testIFunction() {
    IFunction1DTest_Function function;
    std::vector<double> x(10);
    for (size_t i = 0; i < x.size(); ++i) {
      x[i] = 1.0 + 0.1 * double(i);
    }
    FunctionDomain1DVector domain(x);
    FunctionValues values(domain);
    function.function(domain, values);
    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT_EQUALS(values.getCalculated(i),
                       A * (1.0 + 0.1 * double(i)) + B);
    }

    IFunction1DTest_Jacobian jacobian(10, 2);
    function.functionDeriv(domain, jacobian);
    for (size_t i = 0; i < domain.size(); ++i) {
      TS_ASSERT_EQUALS(jacobian.get(i, 0), 1.0 + 0.1 * double(i));
      TS_ASSERT_EQUALS(jacobian.get(i, 1), 1.0);
    }
  }
};

#endif /*IFUNCTION1DTEST_H_*/
