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
  // This is a very specific test for a bug that is now fixed to prevent
  // regression
  void test_FunctionFactory_notification_is_released() {

    // create a FunctionBrowser
    auto fpBrowser = std::make_unique<MantidQt::MantidWidgets::FitPropertyBrowser>();
    // initialise it - this adds an observer on the function factory update
    // message
    fpBrowser->init();
    // delete the FunctionBrowser
    fpBrowser.reset();
    // Make sure the FunctionFactory does not have a dead link as an observer
    TS_ASSERT_THROWS_NOTHING(FunctionFactory::Instance().unsubscribe("FitPropertyBrowserTest_Funct");)
  }
};
