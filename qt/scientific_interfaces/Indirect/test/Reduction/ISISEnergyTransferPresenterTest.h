// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Reduction/ISISEnergyTransferPresenter.h"
#include "Reduction/IndirectDataReduction.h"

#include "MantidAPI/AlgorithmProperties.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace MantidQt::CustomInterfaces;
using MantidQt::API::BatchAlgorithmRunner;
using namespace testing;

namespace {

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> defaultGroupingProps() {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  Mantid::API::AlgorithmProperties::update("GroupingMethod", std::string("IPF"), *properties);
  return properties;
}

} // namespace

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockIETModel : public IETModel {

public:
  MOCK_METHOD3(runIETAlgorithm, std::string(MantidQt::API::BatchAlgorithmRunner *, InstrumentData, IETRunData));
};

class MockIETView : public IETView {
public:
  MOCK_CONST_METHOD0(getRunData, IETRunData());
  MOCK_CONST_METHOD0(getPlotData, IETPlotData());
  MOCK_CONST_METHOD0(getSaveData, IETSaveData());

  MOCK_CONST_METHOD0(getCustomGrouping, std::string());

  MOCK_CONST_METHOD0(getGroupOutputOption, std::string());
  MOCK_CONST_METHOD0(getPlotOptionsView, OutputPlotOptionsView *());
  MOCK_CONST_METHOD0(getGroupOutputCheckbox, bool());

  MOCK_CONST_METHOD0(getFirstFilename, std::string());

  MOCK_CONST_METHOD0(isRunFilesValid, bool());
  MOCK_CONST_METHOD1(validateCalibrationFileType, void(UserInputValidator &));
  MOCK_CONST_METHOD1(validateRebinString, void(UserInputValidator &uiv));

  MOCK_CONST_METHOD0(showRebinWidthPrompt, bool());
  MOCK_CONST_METHOD3(showSaveCustomGroupingDialog,
                     void(std::string const &customGroupingOutput, std::string const &defaultGroupingFilename,
                          std::string const &saveDirectory));
  MOCK_CONST_METHOD1(displayWarning, void(std::string const &message));

  MOCK_METHOD1(setBackgroundSectionVisible, void(bool visible));
  MOCK_METHOD1(setPlotTimeSectionVisible, void(bool visible));
  MOCK_METHOD1(setPlottingOptionsVisible, void(bool visible));
  MOCK_METHOD1(setScaleFactorVisible, void(bool visible));
  MOCK_METHOD1(setAclimaxSaveVisible, void(bool visible));
  MOCK_METHOD1(setNXSPEVisible, void(bool visible));
  MOCK_METHOD1(setFoldMultipleFramesVisible, void(bool visible));
  MOCK_METHOD1(setOutputInCm1Visible, void(bool visible));
  MOCK_METHOD1(setGroupOutputCheckBoxVisible, void(bool visible));
  MOCK_METHOD1(setGroupOutputDropdownVisible, void(bool visible));

  MOCK_METHOD1(setDetailedBalance, void(double detailedBalance));
  MOCK_METHOD1(setRunFilesEnabled, void(bool enable));
  MOCK_METHOD1(setSingleRebin, void(bool enable));
  MOCK_METHOD1(setMultipleRebin, void(bool enable));
  MOCK_METHOD1(setSaveEnabled, void(bool enable));
  MOCK_METHOD1(setPlotTimeIsPlotting, void(bool plotting));
  MOCK_METHOD2(setFileExtensionsByName, void(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes));
  MOCK_METHOD1(setOutputWorkspaces, void(std::vector<std::string> const &outputWorkspaces));

  MOCK_METHOD1(setInstrumentDefault, void(InstrumentData const &instrumentDetails));
};

class MockIndirectDataReduction : public IIndirectDataReduction {};

class IETPresenterTest : public CxxTest::TestSuite {
public:
  static IETPresenterTest *createSuite() { return new IETPresenterTest(); }
  static void destroySuite(IETPresenterTest *suite) { delete suite; }

  void setUp() override {
    auto model = std::make_unique<NiceMock<MockIETModel>>();

    m_view = std::make_unique<NiceMock<MockIETView>>();
    m_model = model.get();
    m_idrUI = std::make_unique<NiceMock<MockIndirectDataReduction>>();

    m_presenter = std::make_unique<IETPresenter>(m_idrUI.get(), m_view.get(), std::move(model));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_idrUI));

    m_presenter.reset();
  }

  void test_fetch_instrument_data() {
    ON_CALL(*m_model, runIETAlgorithm(_, _, _)).WillByDefault(Return(""));
    ON_CALL(*m_view, getRunData());

    ExpectationSet expectRunData = EXPECT_CALL(*m_view, getRunData()).Times(1);
    ExpectationSet expectRunAlgo = EXPECT_CALL(*m_model, runIETAlgorithm(_, _, _)).Times(1).After(expectRunData);

    m_presenter->run();
  }

private:
  std::unique_ptr<IETPresenter> m_presenter;

  std::unique_ptr<NiceMock<MockIETView>> m_view;
  NiceMock<MockIETModel> *m_model;
  std::unique_ptr<NiceMock<MockIndirectDataReduction>> m_idrUI;
};