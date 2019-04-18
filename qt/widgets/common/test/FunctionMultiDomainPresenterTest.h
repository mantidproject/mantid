// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +

#ifndef MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_
#define MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_

#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/IFunctionView.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IFunction.h"
#include <cxxtest/TestSuite.h>
#include <QApplication>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace Mantid::Kernel;

/// This QApplication object is required to construct the view
class QApplicationHolder : CxxTest::GlobalFixture {
public:
  bool setUpWorld() override {
    int argc(0);
    char **argv = {};
    m_app = new QApplication(argc, argv);
    return true;
  }

  bool tearDownWorld() override {
    delete m_app;
    return true;
  }

private:
  QApplication *m_app;
};

static QApplicationHolder MAIN_QAPPLICATION;

class MockFunctionView : public IFunctionView {
public:
  void clear() override {
    m_params.clear();
    m_errors.clear();
  }
  void setFunction(IFunction_sptr fun) override {
    if (fun) {
      for (size_t i = 0; i < fun->nParams(); ++i) {
        auto name = fun->parameterName(i);
        m_params[name] = fun->getParameter(i);
        m_errors[name] = fun->getError(i);
      }
    }
    else {
      clear();
    }
  }
  bool hasFunction() const override { return !m_params.empty(); }
  void setParameter(const QString &paramName, double value) override {
    m_params[paramName.toStdString()] = value;
  }
  void setParamError(const QString &paramName, double error) override {
    m_errors[paramName.toStdString()] = error;
  }
  double getParameter(const QString &paramName) const override {
    return m_params.at(paramName.toStdString());
  }
  void setErrorsEnabled(bool enabled) override { m_areErrorsEnabled = enabled; }
  void clearErrors() override {
    for (auto &it : m_errors) {
      it.second = 0.0;
    }
  }
  boost::optional<QString> currentFunctionIndex() const override {
    return boost::optional<QString>();
  }
private:
  std::map<std::string, double> m_params;
  std::map<std::string, double> m_errors;
  bool m_areErrorsEnabled{ true };
};

class FunctionMultiDomainPrenterTest : public CxxTest::TestSuite {

public:
  static FunctionMultiDomainPrenterTest *createSuite() {
    return new FunctionMultiDomainPrenterTest;
  }
  static void destroySuite(FunctionMultiDomainPrenterTest *suite) { delete suite; }

  FunctionMultiDomainPrenterTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_empty() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    TS_ASSERT(!presenter.getFitFunction());
  }

  void test_simple() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=LinearBackground,A0=1,A1=2");
    TS_ASSERT_DELTA(view->getParameter("A0"), 1.0, 1e-15);
    TS_ASSERT_DELTA(view->getParameter("A1"), 2.0, 1e-15);
    auto fun = presenter.getFitFunction();
    TS_ASSERT(fun);
    if (!fun) return;
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
  }
};

#endif // MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_
