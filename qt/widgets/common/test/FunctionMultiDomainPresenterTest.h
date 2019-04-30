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
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
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
  // Mock implementation of the interface
  void clear() override {
    m_function = IFunction_sptr();
  }
  void setFunction(IFunction_sptr fun) override {
    m_function = fun->clone();
  }
  bool hasFunction() const override { return bool(m_function); }
  void setParameter(const QString &paramName, double value) override {
    m_function->setParameter(paramName.toStdString(), value);
  }
  void setParamError(const QString &paramName, double error) override {
    auto const i = m_function->parameterIndex(paramName.toStdString());
    m_function->setError(i, error);
  }
  double getParameter(const QString &paramName) const override {
    return m_function->getParameter(paramName.toStdString());
  }
  void setErrorsEnabled(bool enabled) override { m_areErrorsEnabled = enabled; }
  void clearErrors() override {
    for (size_t i = 0; i < m_function->nParams(); ++i) {
      m_function->setError(i, 0.0);
    }
  }
  boost::optional<QString> currentFunctionIndex() const override {
    return m_currentFunctionIndex;
  }

  void setParameterTie(const QString &paramName, const QString &tie) override {
    if (!tie.isEmpty()) {
      m_function->tie(paramName.toStdString(), tie.toStdString());
    }
    else {
      m_function->removeTie(paramName.toStdString());
    }
  }

  // Mock user action
  void addFunction(const QString &prefix, const QString& funStr) {
    m_currentFunctionIndex = prefix;
    if (prefix.isEmpty()) {
      auto fun = m_function ? m_function->asString() + ";" : "";
      m_function = FunctionFactory::Instance().createInitialized(fun + funStr.toStdString());
    } else {
      auto parentFun = boost::dynamic_pointer_cast<CompositeFunction>(
        getFunctionWithPrefix(prefix, m_function));
      parentFun->addFunction(FunctionFactory::Instance().createInitialized(funStr.toStdString()));
    }
    emit functionAdded(funStr);
  }

  void userSetsParameterTie(const QString &paramName, const QString &tie) {
    setParameterTie(paramName, tie);
    emit parameterTieChanged(paramName, tie);
  }

  IFunction_sptr getFunction() const { return m_function; }
private:
  IFunction_sptr m_function;
  boost::optional<QString> m_currentFunctionIndex;
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

  void test_setFunction() {
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

  void test_view_addFunction() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground;(name=FlatBackground,A0=1;name=FlatBackground,A0=2)");
    view->addFunction("f1.", "name=LinearBackground");
    auto newFun = presenter.getFunction()->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_addFunction_top_level() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction()->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_addFunction_to_empty() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_multi_addFunction() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    presenter.setFunctionString("name=FlatBackground;(name=FlatBackground,A0=1;name=FlatBackground,A0=2)");
    view->addFunction("f1.", "name=LinearBackground");
    auto newFun = presenter.getFunction()->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(0)->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1)->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(2)->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_multi_addFunction_top_level() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    presenter.setFunctionString("name=FlatBackground");
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction()->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(0)->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1)->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(2)->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_multi_addFunction_to_empty() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(0);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_setCurrentDataset() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    presenter.setFunctionString("name=FlatBackground");
    presenter.setParameter("A0", 1.0);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 1.0);
    presenter.setCurrentDataset(1);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 0.0);
    presenter.setParameter("A0", 2.0);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 2.0);
    presenter.setCurrentDataset(2);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 0.0);
    presenter.setParameter("A0", 3.0);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 3.0);
    presenter.setCurrentDataset(0);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 1.0);
    presenter.setCurrentDataset(1);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 2.0);
    presenter.setCurrentDataset(2);
    TS_ASSERT_EQUALS(view->getParameter("A0"), 3.0);
  }

  void test_setCurrentDataset_composite() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    presenter.setFunctionString("name=FlatBackground;name=FlatBackground");
    presenter.setParameter("f1.A0", 1.0);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 1.0);
    presenter.setCurrentDataset(1);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 0.0);
    presenter.setParameter("f1.A0", 2.0);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 2.0);
    presenter.setCurrentDataset(2);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 0.0);
    presenter.setParameter("f1.A0", 3.0);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 3.0);
    presenter.setCurrentDataset(0);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 1.0);
    presenter.setCurrentDataset(1);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 2.0);
    presenter.setCurrentDataset(2);
    TS_ASSERT_EQUALS(view->getParameter("f1.A0"), 3.0);
  }

  void test_view_set_tie() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground;name=LinearBackground");
    view->userSetsParameterTie("f1.A0", "1.0");
    TS_ASSERT(presenter.isParameterFixed("f1.A0"));
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "");
    view->userSetsParameterTie("f1.A0", "");
    TS_ASSERT(!presenter.isParameterFixed("f1.A0"));
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "");
    view->userSetsParameterTie("f1.A0", "f0.A0");
    TS_ASSERT(!presenter.isParameterFixed("f1.A0"));
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "f0.A0");
    auto fun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(fun->getParameterStatus(1), IFunction::Tied);
    view->userSetsParameterTie("f1.A0", "f1.A0=f1.A0");
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "f1.A0");
  }

  void test_view_set_tie_multi() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    presenter.setFunctionString("name=FlatBackground;name=LinearBackground");
    view->userSetsParameterTie("f1.A0", "1.0");
    TS_ASSERT(presenter.isParameterFixed("f1.A0"));
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "");
    view->userSetsParameterTie("f1.A0", "");
    TS_ASSERT(!presenter.isParameterFixed("f1.A0"));
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "");
    view->userSetsParameterTie("f1.A0", "f0.A0");
    TS_ASSERT(!presenter.isParameterFixed("f1.A0"));
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "f0.A0");
    auto fun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(fun->getParameterStatus(1), IFunction::Tied);
    TS_ASSERT_EQUALS(presenter.getLocalParameterTie("f1.A0", 0), "f0.A0");
    TS_ASSERT_EQUALS(presenter.getLocalParameterTie("f1.A0", 1), "");
    presenter.setCurrentDataset(2);
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "");
    presenter.setCurrentDataset(0);
    TS_ASSERT_EQUALS(presenter.getParameterTie("f1.A0"), "f0.A0");
  }

  void test_presenter_set_tie() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground;name=LinearBackground");
    auto viewFun = view->getFunction();
    presenter.setLocalParameterTie("f1.A0", 0, "1.0");
    TS_ASSERT(viewFun->isFixed(1));
    presenter.setLocalParameterTie("f1.A0", 0, "f0.A0");
    TS_ASSERT(!viewFun->isFixed(1));
    TS_ASSERT(viewFun->getTie(1));
    TS_ASSERT_EQUALS(viewFun->getTie(1)->asString(), "f1.A0=f0.A0");
  }

  void test_presenter_set_tie_multi() {
    auto view = make_unique<MockFunctionView>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    presenter.setFunctionString("name=FlatBackground;name=LinearBackground");
    auto viewFun = view->getFunction();
    presenter.setLocalParameterTie("f1.A0", 0, "1.0");
    TS_ASSERT(viewFun->isFixed(1));
    presenter.setLocalParameterTie("f1.A0", 0, "f0.A0");
    TS_ASSERT(!viewFun->isFixed(1));
    TS_ASSERT(viewFun->getTie(1));
    TS_ASSERT_EQUALS(viewFun->getTie(1)->asString(), "f1.A0=f0.A0");
    presenter.setCurrentDataset(2);
    TS_ASSERT(!viewFun->getTie(1));
    presenter.setLocalParameterTie("f0.A0", 2, "1.0");
    presenter.setCurrentDataset(0);
    TS_ASSERT(!viewFun->isFixed(0));
    TS_ASSERT_EQUALS(viewFun->getTie(1)->asString(), "f1.A0=f0.A0");
  }
};

#endif // MANTIDWIDGETS_FUNCTIONMULTIDOMAINPRENTERTEST_H_
