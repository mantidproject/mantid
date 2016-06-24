#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidAPI/IFunction.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidQtCustomInterfaces/Muon/MuonAnalysisFitFunctionPresenter.h"
#include "MantidQtMantidWidgets/IFunctionBrowser.h"
#include "MantidQtMantidWidgets/IMuonFitFunctionControl.h"

using MantidQt::CustomInterfaces::MuonAnalysisFitFunctionPresenter;
using MantidQt::MantidWidgets::IFunctionBrowser;
using MantidQt::MantidWidgets::IMuonFitFunctionControl;
using namespace testing;

// Mock function browser widget
class MockFunctionBrowser : public IFunctionBrowser {
public:
  QString getFunctionString() override { return QString("Test function"); }
  MOCK_METHOD0(functionStructureChanged, void());
  MOCK_METHOD1(updateParameters, void(const Mantid::API::IFunction &));
  MOCK_METHOD2(parameterChanged, void(const QString &, const QString &));
  MOCK_CONST_METHOD2(getParameter, double(const QString &, const QString &));
  MOCK_METHOD0(clear, void());
  MOCK_METHOD1(setErrorsEnabled, void(bool));
  MOCK_METHOD0(clearErrors, void());
  MOCK_METHOD1(setFunction, void(const QString &));
  MOCK_METHOD1(setNumberOfDatasets, void(int));
};

// Mock muon fit property browser
class MockFitFunctionControl : public IMuonFitFunctionControl {
public:
  MOCK_METHOD1(setFunction, void(const QString &));
  MOCK_METHOD0(runFit, void());
  MOCK_METHOD0(runSequentialFit, void());
  MOCK_METHOD0(functionUpdateRequested, void());
  MOCK_METHOD1(functionUpdateAndFitRequested, void(bool));
  MOCK_CONST_METHOD0(getFunction, Mantid::API::IFunction_sptr());
  MOCK_METHOD3(setParameterValue,
               void(const QString &, const QString &, double));
};

class MuonAnalysisFitFunctionPresenterTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MuonAnalysisFitFunctionPresenterTest *createSuite() {
    return new MuonAnalysisFitFunctionPresenterTest();
  }
  static void destroySuite(MuonAnalysisFitFunctionPresenterTest *suite) {
    delete suite;
  }

  MuonAnalysisFitFunctionPresenterTest() {
    Mantid::API::FrameworkManager::Instance(); // To make sure everything is
                                               // initialized
  }

  /// Create a test function
  Mantid::API::IFunction_sptr createFunction() {
    return Mantid::API::FunctionFactory::Instance().createFunction("Gaussian");
  }

  /// Run before each test to create mock objects
  void setUp() override {
    m_funcBrowser = new NiceMock<MockFunctionBrowser>();
    m_fitBrowser = new NiceMock<MockFitFunctionControl>();
    m_presenter = new MuonAnalysisFitFunctionPresenter(nullptr, m_fitBrowser,
                                                       m_funcBrowser);
    testString = QString("Test function");
  }

  /// Run after each test to check expectations and remove mocks
  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_funcBrowser));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_fitBrowser));
    delete m_funcBrowser;
    delete m_fitBrowser;
    delete m_presenter;
  }

  void test_updateFunction() {
    EXPECT_CALL(*m_fitBrowser, setFunction(testString)).Times(1);
    m_presenter->updateFunction();
  }

  void test_updateFunctionAndFit_nonSequential() {
    EXPECT_CALL(*m_fitBrowser, setFunction(testString)).Times(1);
    EXPECT_CALL(*m_fitBrowser, runFit()).Times(1);
    m_presenter->updateFunctionAndFit(false);
  }

  void test_updateFunctionAndFit_sequential() {
    EXPECT_CALL(*m_fitBrowser, setFunction(testString)).Times(1);
    EXPECT_CALL(*m_fitBrowser, runSequentialFit()).Times(1);
    m_presenter->updateFunctionAndFit(true);
  }

  void test_handleFitFinished() {
    doTest_HandleFitFinishedOrUndone("MUSR00015189; Pair; long; Asym; 1; #1");
  }

  void test_handleFitUndone() {
    EXPECT_CALL(*m_funcBrowser, clearErrors()).Times(1);
    doTest_HandleFitFinishedOrUndone("");
  }

  void test_handleParameterEdited() {
    const double paramValue = 12.345;
    const QString funcIndex = "f0.", paramName = "A0";
    ON_CALL(*m_funcBrowser, getParameter(funcIndex, paramName))
        .WillByDefault(Return(paramValue));
    EXPECT_CALL(*m_funcBrowser, getParameter(funcIndex, paramName)).Times(1);
    EXPECT_CALL(*m_fitBrowser,
                setParameterValue(funcIndex, paramName, paramValue))
        .Times(1);
    m_presenter->handleParameterEdited(funcIndex, paramName);
  }

  void test_handleModelCleared() {
    EXPECT_CALL(*m_funcBrowser, clear()).Times(1);
    m_presenter->handleModelCleared();
  }

  void test_handleErrorsEnabled_On() {
    EXPECT_CALL(*m_funcBrowser, setErrorsEnabled(true)).Times(1);
    m_presenter->handleErrorsEnabled(true);
  }

  void test_handleErrorsEnabled_Off() {
    EXPECT_CALL(*m_funcBrowser, setErrorsEnabled(false)).Times(1);
    m_presenter->handleErrorsEnabled(false);
  }

  void test_handleFunctionLoaded() {
    const QString funcString("some function string");
    EXPECT_CALL(*m_funcBrowser, clear()).Times(1);
    EXPECT_CALL(*m_funcBrowser, setFunction(funcString)).Times(1);
    m_presenter->handleFunctionLoaded(funcString);
  }

  void test_updateNumberOfDatasets() {
    const int nDatasets(21);
    EXPECT_CALL(*m_funcBrowser, setNumberOfDatasets(nDatasets)).Times(1);
    m_presenter->updateNumberOfDatasets(nDatasets);
  }

private:
  /// Run a test of "handleFitFinished" with the given workspace name
  void doTest_HandleFitFinishedOrUndone(const QString &wsName) {
    const auto function = createFunction();
    ON_CALL(*m_fitBrowser, getFunction()).WillByDefault(Return(function));
    EXPECT_CALL(*m_fitBrowser, getFunction()).Times(1);
    EXPECT_CALL(*m_funcBrowser, updateParameters(testing::Ref(*function)))
        .Times(1);
    m_presenter->handleFitFinished(wsName);
  }

  QString testString;
  MockFunctionBrowser *m_funcBrowser;
  MockFitFunctionControl *m_fitBrowser;
  MuonAnalysisFitFunctionPresenter *m_presenter;
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTERTEST_H_ */