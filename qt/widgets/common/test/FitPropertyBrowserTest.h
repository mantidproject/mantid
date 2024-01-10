// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction1D.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/ParamFunction.h"
#include "MantidAPI/WorkspaceFactory.h"
#include "MantidQtWidgets/Common/FitPropertyBrowser.h"
#include "MantidQtWidgets/Common/PropertyHandler.h"
#include "MantidQtWidgets/Common/QtPropertyBrowser/qtpropertybrowser.h"
#include <QApplication>
#include <QSignalSpy>
#include <QString>
#include <QTimer>
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
  static FitPropertyBrowserTest *createSuite() { return new FitPropertyBrowserTest(); }
  static void destroySuite(FitPropertyBrowserTest *suite) { delete suite; }

  void setUp() override { // create a FunctionBrowser
    m_fitPropertyBrowser = std::make_unique<MantidQt::MantidWidgets::FitPropertyBrowser>();
    m_algSignalSpyFailed = std::make_unique<QSignalSpy>(m_fitPropertyBrowser.get(),
                                                        &MantidQt::MantidWidgets::FitPropertyBrowser::algorithmFailed);
    m_algSignalSpyFinished = std::make_unique<QSignalSpy>(
        m_fitPropertyBrowser.get(), &MantidQt::MantidWidgets::FitPropertyBrowser::algorithmFinished);
  }

  void tearDown() override {
    m_fitPropertyBrowser.reset();
    std::cout << "Tear down: fpb reset" << std::endl;
    Mantid::API::AnalysisDataService::Instance().clear();
    std::cout << "Tear down: ads cleared" << std::endl;
  }

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

  void test_removeFunctionRemovesTie() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;"
                                                  "name=Gaussian,Height=12.0,PeakCentre=0.245,Sigma=0.135;"
                                                  "ties=(f0.Height=f1.Height)");
    auto f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    auto f1Handler = m_fitPropertyBrowser->getPeakHandler(QString("f1"));
    TS_ASSERT(f0Handler->hasTies());
    m_fitPropertyBrowser->removeFunction(f1Handler);
    f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    TS_ASSERT(!f0Handler->hasTies());
  }

  void test_removeFunctionRenamesOtherFunction() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction(
        "name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;name=FlatBackground,A0=10;"
        "ties=(f0.Height=f1.A0)");
    auto f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    TS_ASSERT_EQUALS(QString("f0-Gaussian"), f0Handler->functionName());
    m_fitPropertyBrowser->removeFunction(f0Handler);
    // f0 should now be the flat background function
    f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    TS_ASSERT(!f0Handler->hasTies());
    TS_ASSERT_EQUALS(QString("f0-FlatBackground"), f0Handler->functionName());
  }

  void test_removeFunctionUpdatesTieString() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction(
        "name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;name=FlatBackground,A0=10;"
        "name=Gaussian,Height=10.0,PeakCentre=-0.555,Sigma=0.135;ties=(f0.Height=f2.Sigma)");
    auto cf = m_fitPropertyBrowser->compositeFunction();
    auto f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    auto f1Handler = m_fitPropertyBrowser->getPeakHandler(QString("f1"));
    auto tie = cf->getTie(cf->parameterIndex("f0.Height"));
    std::string tie_str = tie->asString();
    TS_ASSERT_EQUALS(tie_str.substr(tie_str.find("=") + 1), "f2.Sigma");

    m_fitPropertyBrowser->removeFunction(f1Handler);

    TS_ASSERT(f0Handler->hasTies());
    tie = cf->getTie(cf->parameterIndex("f0.Height"));
    tie_str = tie->asString();
    TS_ASSERT_EQUALS(tie_str.substr(tie_str.find("=") + 1), "f1.Sigma");
  }

  void test_circularTieIsNotAdded() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;"
                                                  "name=Gaussian,Height=12.0,PeakCentre=0.245,Sigma=0.135;"
                                                  "ties=(f0.Height=f1.Height)");

    auto f1Handler = m_fitPropertyBrowser->getPeakHandler(QString("f1"));

    expect_and_close_message_box();

    // bad circualr tie
    f1Handler->addTie("f1.Height=f0.Height");
    TS_ASSERT(!f1Handler->hasTies());
  }

  void test_selfTieIsNotAdded() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135");

    auto f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));

    expect_and_close_message_box();

    // bad self tie
    f0Handler->addTie("f0.Height=f0.Height");
    TS_ASSERT(!f0Handler->hasTies());
  }

  void test_tiesAreAppliedOnLoad() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;"
                                                  "name=Gaussian,Height=12.0,PeakCentre=0.245,Sigma=0.135;"
                                                  "ties=(f0.Height=f1.Height)");

    auto f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    auto heightProperty = f0Handler->getParameterProperty(QString("Height"));
    const auto height = heightProperty->valueText().toDouble();
    TS_ASSERT_EQUALS(height, 12);
  }

  void test_tiesAreAppliedOnNewTie() {
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=Gaussian,Height=10.0,PeakCentre=-0.145,Sigma=0.135;"
                                                  "name=Gaussian,Height=12.0,PeakCentre=0.245,Sigma=0.135;");

    auto f0Handler = m_fitPropertyBrowser->getPeakHandler(QString("f0"));
    auto heightProperty = f0Handler->getParameterProperty(QString("Height"));
    auto height = heightProperty->valueText().toDouble();
    TS_ASSERT_EQUALS(height, 10);

    f0Handler->addTie("f0.Height=f1.Height");

    heightProperty = f0Handler->getParameterProperty(QString("Height"));
    height = heightProperty->valueText().toDouble();
    TS_ASSERT_EQUALS(height, 12);
  }

  void test_alg_failed_signal_emmitted_upon_exception() {
    m_algSignalSpyFailed->clear();
    m_algSignalSpyFinished->clear();
    auto ws = Mantid::API::WorkspaceFactory::Instance().create("Workspace2D", 5, 5, 5);
    Mantid::API::AnalysisDataService::Instance().addOrReplace("test_ws_name", ws);
    m_fitPropertyBrowser->init();
    m_fitPropertyBrowser->createCompositeFunction("name=UserFunction;");
    m_fitPropertyBrowser->setWorkspaceName("test_ws_name");
    m_fitPropertyBrowser->fit();
    std::cout << "Reached end of fitting" << std::endl;
    TS_ASSERT_EQUALS(m_algSignalSpyFailed->count(), 1);
    std::cout << "Compared algorithmFailed count" << std::endl;
    TS_ASSERT_EQUALS(m_algSignalSpyFinished->count(), 0);
    std::cout << "Compared algorithmFinished count" << std::endl;
  }

private:
  std::unique_ptr<MantidQt::MantidWidgets::FitPropertyBrowser> m_fitPropertyBrowser;
  std::unique_ptr<QSignalSpy> m_algSignalSpyFailed;
  std::unique_ptr<QSignalSpy> m_algSignalSpyFinished;

  void expect_and_close_message_box() {
    QTimer::singleShot(0, [=]() {
      auto msgBox = QApplication::activeModalWidget();
      if (msgBox) {
        msgBox->close();
      } else {
        TS_FAIL("Expected critical error message box to be shown");
      }
    });
  }
};
