// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include <QString>
#include <cxxtest/TestSuite.h>

using namespace Mantid::API;

class FitPropertyBrowserTest_Funct : public ParamFunction, public IFunction1D {
public:
  FitPropertyBrowserTest_Funct() {
    declareParameter("b0");
    declareParameter("b1");
  }

  std::string name() const override { return "FitPropertyBrowserTest_Funct"; }

  void function1D(double *out, const double *xValues, const size_t nData) const override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
  void functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) override {
    UNUSED_ARG(out);
    UNUSED_ARG(xValues);
    UNUSED_ARG(nData);
  }
};

DECLARE_FUNCTION(FitPropertyBrowserTest_Funct)

class FitPropertyBrowserTest : public CxxTest::TestSuite {
public:
  static FitPropertyBrowserTest *createSuite() { return new FitPropertyBrowserTest; }
  static void destroySuite(FitPropertyBrowserTest *suite) { delete suite; }

  void setUp() override { // create a FunctionBrowser
    m_fitPropertyBrowser = std::make_unique<MantidQt::MantidWidgets::FitPropertyBrowser>();
  }

  void tearDown() override { m_fitPropertyBrowser.reset(); }

  // This is a very specific test for a bug that is now fixed to prevent regression
  void test_FunctionFactory_notification_is_released() {

    // create a FunctionBrowser
    auto fpBrowser = std::make_unique<MantidQt::MantidWidgets::FitPropertyBrowser>();
    // initialise it - this adds an observer on the function factory update message
    fpBrowser->init();
    // delete the FunctionBrowser
    fpBrowser.reset();
    // Make sure the FunctionFactory does not have a dead link as an observer
    TS_ASSERT_THROWS_NOTHING(FunctionFactory::Instance().unsubscribe("FitPropertyBrowserTest_Funct");)
  }

  void test_getOldExpressionAsString_returns_empty_string_when_tie_is_null() {
    // initialise fitPropertyBrowser- this adds an observer on the function factory update message
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=100,PeakCentre=1.45,Sigma=0.2;");
    QString parameterName = "f0.Height";
    auto oldExpString = m_fitPropertyBrowser->getOldExpressionAsString(parameterName);
    TS_ASSERT_EQUALS(oldExpString.toStdString(), "");
  }

  void test_getOldExpressionAsString_returns_empty_string_when_parameter_is_null() {
    // initialise fitPropertyBrowser- this adds an observer on the function factory update message
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=100,PeakCentre=1.45,Sigma=0.2;");
    QString parameterName = "InvalidParameterName";
    auto oldExpString = m_fitPropertyBrowser->getOldExpressionAsString(parameterName);
    TS_ASSERT_EQUALS(oldExpString.toStdString(), "");
  }

  void test_getOldExpressionAsString_returns_function_expression() {
    // Note this test only works for a tie to a function (e.g. f0.Height=f1.Height) but not a constant (e.g.
    // f0.Height=5.0)

    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction(
        "name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;name=Gaussian,Height=12.0,PeakCentre=0.245,Sigma=0."
        "135;ties=(f0.Height=f1.Height)");
    QString parameterName = "f0.Height";
    const std::string expectedTieExpression = "f1.Height";
    auto oldExpString = m_fitPropertyBrowser->getOldExpressionAsString(parameterName);
    TS_ASSERT_EQUALS(oldExpString.toStdString(), expectedTieExpression);
  }

private:
  std::unique_ptr<MantidQt::MantidWidgets::FitPropertyBrowser> m_fitPropertyBrowser;
};
