// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/CompositeFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MultiDomainFunction.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FunctionBrowser/FunctionBrowserUtils.h"
#include "MantidQtWidgets/Common/FunctionModel.h"
#include "MantidQtWidgets/Common/FunctionMultiDomainPresenter.h"
#include "MantidQtWidgets/Common/IFunctionView.h"
#include <QApplication>
#include <cxxtest/TestSuite.h>

#include <gmock/gmock.h>

using namespace MantidQt::MantidWidgets;
using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockFunctionView : public IFunctionView {
public:
  // Mock implementation of the interface
  void clear() override { m_function = IFunction_sptr(); }
  void setFunction(IFunction_sptr fun) override {
    if (fun)
      m_function = fun->clone();
    else
      m_function = fun;
  }
  bool hasFunction() const override { return bool(m_function); }
  void setParameter(const QString &paramName, double value) override {
    m_function->setParameter(paramName.toStdString(), value);
  }
  void setParameterError(const QString &paramName, double error) override {
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
  std::optional<QString> currentFunctionIndex() const override { return m_currentFunctionIndex; }

  void setParameterTie(const QString &paramName, const QString &tie) override {
    if (!tie.isEmpty()) {
      m_function->tie(paramName.toStdString(), tie.toStdString());
    } else {
      m_function->removeTie(paramName.toStdString());
    }
  }

  void setParameterConstraint(const QString & /*paramName*/, const QString & /*constraint*/) override {}

  void setGlobalParameters(const QStringList &globals) override { m_globals = globals; }
  void functionHelpRequested() { emit functionHelpRequest(); }
  MOCK_METHOD0(getSelectedFunction, IFunction_sptr());
  MOCK_CONST_METHOD1(showFunctionHelp, void(const QString &));

  // Mock user action
  void addFunction(const QString &prefix, const QString &funStr) {
    m_currentFunctionIndex = prefix;
    if (prefix.isEmpty()) {
      auto fun = m_function ? m_function->asString() + ";" : "";
      m_function = FunctionFactory::Instance().createInitialized(fun + funStr.toStdString());
    } else {
      auto parentFun = std::dynamic_pointer_cast<CompositeFunction>(getFunctionWithPrefix(prefix, m_function));
      parentFun->addFunction(FunctionFactory::Instance().createInitialized(funStr.toStdString()));
    }
    emit functionAdded(funStr);
  }

  void userSetsParameterTie(const QString &paramName, const QString &tie) {
    setParameterTie(paramName, tie);
    emit parameterTieChanged(paramName, tie);
  }

  void removeFunction(const QString &prefix) {
    QString parentPrefix;
    int i;
    std::tie(parentPrefix, i) = splitFunctionPrefix(prefix);
    auto fun = std::dynamic_pointer_cast<CompositeFunction>(getFunctionWithPrefix(parentPrefix, m_function));
    if (i >= 0)
      fun->removeFunction(i);
    else
      clear();
    emit functionRemoved(prefix);
  }

  IFunction_sptr getFunction() const { return m_function; }

  MOCK_CONST_METHOD1(getAttribute, IFunction::Attribute(const QString &));

  void attributeChanged() { emit attributePropertyChanged(QString("f0.Q")); }

  friend class FunctionMultiDomainPresenterTest;

private:
  IFunction_sptr m_function;
  std::optional<QString> m_currentFunctionIndex;
  bool m_areErrorsEnabled{true};
  QStringList m_globals;
  MOCK_METHOD2(setDoubleAttribute, void(const QString &, double));
  MOCK_METHOD2(setIntAttribute, void(const QString &, int));
  MOCK_METHOD2(setStringAttribute, void(const QString &, std::string &));
  MOCK_METHOD2(setBooleanAttribute, void(const QString &, bool));
  MOCK_METHOD2(setVectorAttribute, void(const QString &, std::vector<double> &));
};

class FunctionMultiDomainPresenterTest : public CxxTest::TestSuite {

public:
  static FunctionMultiDomainPresenterTest *createSuite() { return new FunctionMultiDomainPresenterTest; }
  static void destroySuite(FunctionMultiDomainPresenterTest *suite) { delete suite; }

  FunctionMultiDomainPresenterTest() {
    // To make sure API is initialized properly
    FrameworkManager::Instance();
  }

  void test_utils_splitFunctionPrefix() {
    QString prefix;
    int i;
    std::tie(prefix, i) = splitFunctionPrefix("");
    TS_ASSERT_EQUALS(prefix, "");
    TS_ASSERT_EQUALS(i, -1);
    std::tie(prefix, i) = splitFunctionPrefix("f0.");
    TS_ASSERT_EQUALS(prefix, "");
    TS_ASSERT_EQUALS(i, 0);
    std::tie(prefix, i) = splitFunctionPrefix("f0.f1.");
    TS_ASSERT_EQUALS(prefix, "f0.");
    TS_ASSERT_EQUALS(i, 1);
    std::tie(prefix, i) = splitFunctionPrefix("f0.f3.f24.");
    TS_ASSERT_EQUALS(prefix, "f0.f3.");
    TS_ASSERT_EQUALS(i, 24);
  }

  void test_empty() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    TS_ASSERT(!presenter.getFitFunction());
  }

  void test_setFunction() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=LinearBackground,A0=1,A1=2");
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 0);
    TS_ASSERT_DELTA(view->getParameter("A0"), 1.0, 1e-15);
    TS_ASSERT_DELTA(view->getParameter("A1"), 2.0, 1e-15);
    auto fun = presenter.getFitFunction();
    TS_ASSERT(fun);
    if (!fun)
      return;
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
  }

  void test_view_addFunction() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground;(name=FlatBackground,A0=1;"
                                "name=FlatBackground,A0=2)");
    view->addFunction("f1.", "name=LinearBackground");
    auto newFun = presenter.getFunction()->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1)->getFunction(2);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_addFunction_top_level() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction()->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction()->getFunction(1);
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_addFunction_to_empty() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_view_multi_addFunction() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(3);
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 3);
    presenter.setFunctionString("name=FlatBackground;(name=FlatBackground,A0=1;"
                                "name=FlatBackground,A0=2)");
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
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

  void test_set_datasets() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    presenter.setNumberOfDatasets(0);
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 0);
    auto fun = presenter.getFunction();
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "FlatBackground");
    auto ffun = presenter.getFitFunction();
    TS_ASSERT(ffun);
    TS_ASSERT_EQUALS(ffun->name(), "FlatBackground");
    presenter.setNumberOfDatasets(2);
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 2);
    ffun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(ffun->nFunctions(), 2);
    presenter.setNumberOfDatasets(0);
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 0);
    fun = presenter.getFunction();
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "FlatBackground");
    ffun = presenter.getFitFunction();
    TS_ASSERT(ffun);
    TS_ASSERT_EQUALS(ffun->name(), "FlatBackground");
  }

  void test_set_datasets_zero() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(0);
    presenter.setFunctionString("name=FlatBackground");
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 0);
    auto fun = presenter.getFunction();
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "FlatBackground");
    auto ffun = presenter.getFitFunction();
    TS_ASSERT(ffun);
    TS_ASSERT_EQUALS(ffun->name(), "FlatBackground");
    TS_ASSERT_THROWS_NOTHING(presenter.setCurrentDataset(0));
  }

  void test_remove_datasets() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    presenter.setNumberOfDatasets(5);
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 5);
    auto ffun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(ffun->getNumberDomains(), 5);
    QList<int> indices;
    indices << 2 << 4 << 1;
    presenter.removeDatasets(indices);
    ffun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(ffun->getNumberDomains(), 2);
    TS_ASSERT_EQUALS(ffun->nFunctions(), 2);
    TS_ASSERT_EQUALS(presenter.getNumberOfDatasets(), 2);
  }

  void test_replace_function() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    presenter.setNumberOfDatasets(2);
    presenter.setFunctionString("name=LinearBackground");
    auto fun = presenter.getFunction();
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
  }

  void test_remove_function_single() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    auto fun = presenter.getFunction();
    TS_ASSERT_EQUALS(fun->name(), "FlatBackground");
    view->removeFunction("");
    fun = presenter.getFunction();
    TS_ASSERT(!fun);
    presenter.setFunctionString("name=FlatBackground;name=LinearBackground");
    view->removeFunction("f0.");
    fun = presenter.getFunction();
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
  }

  void test_remove_function_multi() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    presenter.setNumberOfDatasets(2);
    auto fun = presenter.getFunction();
    TS_ASSERT_EQUALS(fun->name(), "FlatBackground");
    view->removeFunction("");
    fun = presenter.getFunction();
    TS_ASSERT(!fun);
    presenter.setFunctionString("name=FlatBackground;name=LinearBackground");
    view->removeFunction("f0.");
    fun = presenter.getFunction();
    TS_ASSERT(fun);
    TS_ASSERT_EQUALS(fun->name(), "LinearBackground");
    auto ffun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(ffun->nParams(), 4);
  }

  void test_view_addFunction_after_remove() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=FlatBackground");
    view->removeFunction("");
    view->addFunction("", "name=LinearBackground");
    auto newFun = presenter.getFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
    newFun = presenter.getFitFunction();
    TS_ASSERT_EQUALS(newFun->name(), "LinearBackground");
  }

  void test_set_globals() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=LinearBackground");
    presenter.setNumberOfDatasets(3);
    QStringList globals("A1");
    presenter.setGlobalParameters(globals);
    auto fun = presenter.getFitFunction();
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT_EQUALS(fun->getTie(3)->asString(), "f1.A1=f0.A1");
    TS_ASSERT_EQUALS(fun->getTie(5)->asString(), "f2.A1=f0.A1");
    auto locals = presenter.getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A0");
    globals.clear();
    globals << "A0";
    presenter.setGlobalParameters(globals);
    fun = presenter.getFitFunction();
    TS_ASSERT(!fun->getTie(0));
    TS_ASSERT(!fun->getTie(1));
    TS_ASSERT(!fun->getTie(3));
    TS_ASSERT(!fun->getTie(5));
    TS_ASSERT_EQUALS(fun->getTie(2)->asString(), "f1.A0=f0.A0");
    TS_ASSERT_EQUALS(fun->getTie(4)->asString(), "f2.A0=f0.A0");
    locals = presenter.getLocalParameters();
    TS_ASSERT_EQUALS(locals[0], "A1");
  }
  void test_open_function_help_window() {
    auto func = FunctionFactory::Instance().createInitialized("name=LinearBackground");
    QString functionName = QString::fromStdString(func->name());
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());

    EXPECT_CALL(*view, getSelectedFunction()).Times(Exactly(1)).WillOnce(Return(func));
    EXPECT_CALL(*view, showFunctionHelp(functionName)).Times(Exactly(1));

    view->functionHelpRequested();
  }
  void test_open_function_help_window_no_function() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());

    EXPECT_CALL(*view, getSelectedFunction()).Times(Exactly(1)).WillOnce(Return(IFunction_sptr()));
    EXPECT_CALL(*view, showFunctionHelp(_)).Times(Exactly(0));

    view->functionHelpRequested();
  }
  void test_updateMultiDatasetAttributes_updates_view_attributes() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setNumberOfDatasets(1);
    presenter.setFunctionString("name=TeixeiraWaterSQE, Q=3.14, WorkspaceIndex=4, Height=1, "
                                "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                                "constraints=(Height>0, DiffCoeff>0, "
                                "Tau>0);name=FlatBackground;name=LinearBackground");

    auto function = FunctionFactory::Instance().createInitializedMultiDomainFunction(
        "name=TeixeiraWaterSQE, Q=41.3, "
        "Height=1, DiffCoeff=2.3, Tau=1.25, Centre=0, "
        "constraints=(Height>0, DiffCoeff>0, "
        "Tau>0);name=FlatBackground;name=LinearBackground",
        1);
    auto &func = dynamic_cast<IFunction &>(*function);

    // this function has three attributes: NumDeriv (boolean attribute), f0.Q (a
    // double attribute) and f0.WorkspaceIndex (integer attribute) so we should
    // expect those calls
    EXPECT_CALL(*view, setBooleanAttribute(QString("NumDeriv"), false)).Times(Exactly(1));
    EXPECT_CALL(*view, setDoubleAttribute(QString("f0.Q"), 41.3)).Times(Exactly(1));
    EXPECT_CALL(*view, setIntAttribute(QString("f0.WorkspaceIndex"), Mantid::EMPTY_INT())).Times(Exactly(1));

    presenter.updateMultiDatasetAttributes(func);
  }

  void test_attribute_changed_gets_attribute_value_from_view() {
    auto view = std::make_unique<NiceMock<MockFunctionView>>();
    FunctionMultiDomainPresenter presenter(view.get());
    presenter.setFunctionString("name=TeixeiraWaterSQE, Q=3, WorkspaceIndex=4, Height=1, "
                                "DiffCoeff=2.3, Tau=1.25, Centre=0, "
                                "constraints=(Height>0, DiffCoeff>0, "
                                "Tau>0);name=FlatBackground;name=LinearBackground");
    EXPECT_CALL(*view, getAttribute(QString("f0.Q"))).Times(Exactly(1)).WillOnce(Return(IFunction::Attribute()));

    view->attributeChanged();
  }
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
