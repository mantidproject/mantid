// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTERTEST_H_
#define MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "../Muon/MuonAnalysisFitFunctionPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/IFunction.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/IFunctionBrowser.h"
#include "MantidQtWidgets/Common/IMuonFitFunctionModel.h"

using MantidQt::CustomInterfaces::Muon::MultiFitState;
using MantidQt::CustomInterfaces::MuonAnalysisFitFunctionPresenter;
using MantidQt::MantidWidgets::IFunctionBrowser;
using MantidQt::MantidWidgets::IMuonFitFunctionModel;
using namespace testing;

// Mock function browser widget
class MockFunctionBrowser : public IFunctionBrowser {
public:
  MockFunctionBrowser() {
    m_func =
        Mantid::API::FunctionFactory::Instance().createFunction("Gaussian");
  }
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(getFunctionString, QString());
  Mantid::API::IFunction_sptr getGlobalFunction() override { return m_func; }
  MOCK_METHOD0(functionStructureChanged, void());
  MOCK_METHOD1(updateParameters, void(const Mantid::API::IFunction &));
  MOCK_METHOD2(parameterChanged, void(const QString &, const QString &));
  MOCK_METHOD0(clear, void());
  MOCK_METHOD1(setErrorsEnabled, void(bool));
  MOCK_METHOD0(clearErrors, void());
  MOCK_METHOD1(setFunction, void(const QString &));
  MOCK_METHOD1(setNumberOfDatasets, void(int));
  MOCK_METHOD1(setDatasetNames, void(const QStringList &));
  MOCK_METHOD1(updateMultiDatasetParameters,
               void(const Mantid::API::IFunction &));
  MOCK_CONST_METHOD2(isLocalParameterFixed, bool(const QString &, int));
  MOCK_CONST_METHOD2(getLocalParameterValue, double(const QString &, int));
  MOCK_CONST_METHOD2(getLocalParameterTie, QString(const QString &, int));
  MOCK_CONST_METHOD0(getNumberOfDatasets, int());
  MOCK_CONST_METHOD0(getCurrentDataset, int());
  MOCK_METHOD3(setLocalParameterValue, void(const QString &, int, double));
  MOCK_METHOD3(setLocalParameterFixed, void(const QString &, int, bool));
  MOCK_METHOD3(setLocalParameterTie, void(const QString &, int, QString));
  MOCK_METHOD1(setCurrentDataset, void(int));
  MOCK_METHOD3(editLocalParameter,
               void(const QString &parName, const QStringList &wsNames,
                    const std::vector<size_t> &wsIndices));
  GNU_DIAG_ON_SUGGEST_OVERRIDE

private:
  Mantid::API::IFunction_sptr m_func;
};

// Mock muon fit property browser
class MockFitFunctionControl : public IMuonFitFunctionModel {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD1(setFunction, void(const Mantid::API::IFunction_sptr));
  MOCK_METHOD0(runFit, void());
  MOCK_METHOD0(runSequentialFit, void());
  MOCK_METHOD0(functionUpdateRequested, void());
  MOCK_METHOD1(functionUpdateAndFitRequested, void(bool));
  MOCK_CONST_METHOD0(getFunction, Mantid::API::IFunction_sptr());
  MOCK_CONST_METHOD0(getWorkspaceNamesToFit, std::vector<std::string>());
  MOCK_METHOD1(userChangedDatasetIndex, void(int));
  MOCK_METHOD1(setMultiFittingMode, void(bool));
  MOCK_CONST_METHOD0(isMultiFittingMode, bool());
  MOCK_METHOD1(fitRawDataClicked, void(bool));
  MOCK_METHOD0(doRemoveGuess, void());
  MOCK_METHOD0(doPlotGuess, void());
  MOCK_CONST_METHOD0(hasGuess, bool());
  GNU_DIAG_ON_SUGGEST_OVERRIDE
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
    m_presenter->setMultiFitState(
        MantidQt::CustomInterfaces::Muon::MultiFitState::Enabled);
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
    EXPECT_CALL(*m_funcBrowser, getFunctionString())
        .Times(1)
        .WillOnce(Return("Test Function"));
    EXPECT_CALL(*m_fitBrowser, setFunction(m_funcBrowser->getGlobalFunction()))
        .Times(1);
    m_presenter->updateFunction();
  }

  void test_updateFunction_lastFunctionRemoved() {
    EXPECT_CALL(*m_funcBrowser, getFunctionString())
        .Times(1)
        .WillOnce(Return("")); // empty string - last function removed
    EXPECT_CALL(*m_fitBrowser, setFunction(Mantid::API::IFunction_sptr()))
        .Times(1);
    m_presenter->updateFunction();
  }

  void test_updateFunctionAndFit_nonSequential() {
    EXPECT_CALL(*m_funcBrowser, getFunctionString())
        .Times(1)
        .WillOnce(Return("Test Function"));
    EXPECT_CALL(*m_fitBrowser, setFunction(m_funcBrowser->getGlobalFunction()))
        .Times(1);
    EXPECT_CALL(*m_fitBrowser, runFit()).Times(1);
    m_presenter->updateFunctionAndFit(false);
  }

  void test_updateFunctionAndFit_sequential() {
    EXPECT_CALL(*m_funcBrowser, getFunctionString())
        .Times(1)
        .WillOnce(Return("Test Function"));
    EXPECT_CALL(*m_fitBrowser, setFunction(m_funcBrowser->getGlobalFunction()))
        .Times(1);
    EXPECT_CALL(*m_fitBrowser, runSequentialFit()).Times(1);
    m_presenter->updateFunctionAndFit(true);
  }

  void test_handleFitFinished() {
    m_presenter->setMultiFitState(
        MantidQt::CustomInterfaces::Muon::MultiFitState::Enabled);
    doTest_HandleFitFinishedOrUndone("MUSR00015189; Pair; long; Asym; 1; #1",
                                     false);
  }

  void test_handleFitFinished_multiFitDisabled() {
    m_presenter->setMultiFitState(
        MantidQt::CustomInterfaces::Muon::MultiFitState::Disabled);
    doTest_HandleFitFinishedOrUndone("MUSR00015189; Pair; long; Asym; 1; #1",
                                     true);
  }

  void test_handleFitUndone() {
    EXPECT_CALL(*m_funcBrowser, clearErrors()).Times(1);
    doTest_HandleFitFinishedOrUndone("", false);
  }

  void test_handleParameterEdited() {
    const QString funcIndex = "f0.", paramName = "A0";
    EXPECT_CALL(*m_funcBrowser, getFunctionString())
        .Times(1)
        .WillOnce(Return("Test Function"));
    EXPECT_CALL(*m_fitBrowser, setFunction(m_funcBrowser->getGlobalFunction()))
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

  void test_updateNumberOfDatasets() {
    const int nDatasets(21);
    EXPECT_CALL(*m_funcBrowser, clearErrors()).Times(1);
    EXPECT_CALL(*m_funcBrowser, setNumberOfDatasets(nDatasets)).Times(1);
    m_presenter->updateNumberOfDatasets(nDatasets);
  }

  void test_handleDatasetIndexChanged() {
    const int index(2);
    EXPECT_CALL(*m_funcBrowser, setCurrentDataset(index)).Times(1);
    m_presenter->handleDatasetIndexChanged(index);
  }

  void test_setMultiFitMode_On() {
    EXPECT_CALL(*m_fitBrowser, setMultiFittingMode(true)).Times(1);
    m_presenter->setMultiFitState(MultiFitState::Enabled);
  }

  void test_setMultiFitMode_Off() {
    EXPECT_CALL(*m_fitBrowser, setMultiFittingMode(false)).Times(1);
    m_presenter->setMultiFitState(MultiFitState::Disabled);
  }
  void test_setFunctionInModel_multiFitOn_hasGuess() {
    doTest_setFunctionInModel(MultiFitState::Enabled, true);
  }

  void test_setFunctionInModel_multiFitOn_noGuess() {
    doTest_setFunctionInModel(MultiFitState::Enabled, false);
  }

  void test_setFunctionInModel_multiFitOff_hasGuess() {
    doTest_setFunctionInModel(MultiFitState::Disabled, true);
  }

  void test_setFunctionInModel_multiFitOff_noGuess() {
    doTest_setFunctionInModel(MultiFitState::Disabled, false);
  }

private:
  /// Run a test of "handleFitFinished" with the given workspace name
  void doTest_HandleFitFinishedOrUndone(const QString &wsName,
                                        bool compatibility) {
    const int times = compatibility ? 0 : 1;
    const auto &function = createFunction();
    ON_CALL(*m_fitBrowser, getFunction()).WillByDefault(Return(function));
    EXPECT_CALL(*m_fitBrowser, getFunction()).Times(times);
    EXPECT_CALL(*m_funcBrowser,
                updateMultiDatasetParameters(testing::Ref(*function)))
        .Times(times);
    m_presenter->handleFitFinished(wsName);
  }

  /// Run a test of "setFunctionInModel" with the given options
  void doTest_setFunctionInModel(MultiFitState multiState, bool hasGuess) {
    m_presenter->setMultiFitState(multiState);
    EXPECT_CALL(*m_fitBrowser, hasGuess())
        .Times(AnyNumber())
        .WillRepeatedly(Return(hasGuess));
    const int times = multiState == MultiFitState::Enabled && hasGuess ? 1 : 0;
    const auto &function = createFunction();
    {
      testing::InSequence s;
      EXPECT_CALL(*m_fitBrowser, doRemoveGuess()).Times(times);
      EXPECT_CALL(*m_fitBrowser, setFunction(function)).Times(1);
      EXPECT_CALL(*m_fitBrowser, doPlotGuess()).Times(times);
    }
    m_presenter->setFunctionInModel(function);
  }

  MockFunctionBrowser *m_funcBrowser;
  MockFitFunctionControl *m_fitBrowser;
  MuonAnalysisFitFunctionPresenter *m_presenter;
};

#endif /* MANTID_CUSTOMINTERFACES_MUONANALYSISFITFUNCTIONPRESENTERTEST_H_ */
