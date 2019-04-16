// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_FUNCTIONSINGLEDOMAINPRENTERTEST_H_
#define MANTIDWIDGETS_FUNCTIONSINGLEDOMAINPRENTERTEST_H_

#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/IFunctionView.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFunction.h"
#include <cxxtest/TestSuite.h>

#include <QMap>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;

class MyFunctionView : public IFunctionView {
public:
  void clear() override;
  void setFunction(IFunction_sptr fun) override;
  bool hasFunction() const override;
  void setParameter(const QString &funcIndex, const QString &paramName, double value) override;
  void setParamError(const QString &funcIndex, const QString &paramName, double error) override;
  double getParameter(const QString &funcIndex, const QString &paramName) const override;
  void setErrorsEnabled(bool enabled) override;
  void clearErrors() override;
private:

};

class FunctionSingleDomainPrenterTest : public CxxTest::TestSuite {

public:
  static FunctionSingleDomainPrenterTest *createSuite() {
    return new FunctionSingleDomainPrenterTest;
  }
  static void destroySuite(FunctionSingleDomainPrenterTest *suite) { delete suite; }

  FunctionSingleDomainPrenterTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    SingleDomainFunctionModel model;
    TS_ASSERT(!model.getFitFunction());
  }
};

#endif // MANTIDWIDGETS_FUNCTIONSINGLEDOMAINPRENTERTEST_H_
