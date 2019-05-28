// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TEMPFUNCTIONTEST_H_
#define TEMPFUNCTIONTEST_H_

#include "MantidAPI/IFunctionMW.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/TempFunction.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;

class TFT_Funct : public ParamFunction, public IFunctionMW {
public:
  TFT_Funct() {
    declareParameter("c0", 0.0, "this is the famous c0 blah...");
    declareParameter("c1");
    declareParameter("c2");
    declareParameter("c3");
  }

  std::string name() const { return "TFT_Funct"; }

  void functionMW(double *out, const double *xValues,
                  const size_t nData) const {
    double c0 = getParameter("c0");
    double c1 = getParameter("c1");
    double c2 = getParameter("c2");
    double c3 = getParameter("c3");
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i];
      out[i] = c0 + x * (c1 + x * (c2 + x * c3));
    }
  }
  void functionDerivMW(Jacobian *out, const double *xValues,
                       const size_t nData) {
    for (size_t i = 0; i < nData; i++) {
      double x = xValues[i];
      out->set(i, 0, 1.);
      out->set(i, 1, x);
      out->set(i, 2, x * x);
      out->set(i, 3, x * x * x);
    }
  }
};

class TempFunctionTest : public CxxTest::TestSuite {
public:
  void testFunction() {
    TempFunction fun(new TFT_Funct);
    TS_ASSERT_EQUALS(fun.name(), "TFT_Funct");
    TS_ASSERT_EQUALS(fun.nParams(), 4);

    Mantid::API::FunctionDomain1D domain(0.0, 1.0, 10);
    TS_ASSERT_EQUALS(domain.size(), 10);
    TS_ASSERT_EQUALS(domain.getX(0), 0);
    TS_ASSERT_DELTA(domain.getX(9), 1.0, 1e-9);
    TS_ASSERT_EQUALS(domain.getX(1), 1.0 / 9);

    fun.setParameter("c0", 3.0);
    fun.setParameter("c1", 1.0);
    fun.function(domain);

    for (size_t i = 0; i < domain.size(); ++i) {
      double x = domain.getX(i);
      double y = domain.getCalculated(i);
      TS_ASSERT_EQUALS(y, 3.0 + x);
    }
  }

  void test_domain_create() {
    TS_ASSERT_THROWS(Mantid::API::FunctionDomain d(0),
                     const std::invalid_argument &);
    TS_ASSERT_THROWS(Mantid::API::FunctionDomain d(-10),
                     const std::length_error &);
    TS_ASSERT_THROWS_NOTHING(Mantid::API::FunctionDomain d(1));
  }
};

#endif /*TEMPFUNCTIONTEST_H_*/
