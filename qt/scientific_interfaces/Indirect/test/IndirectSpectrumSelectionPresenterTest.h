// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectSpectrumSelectionView : public IndirectSpectrumSelectionView {
public:
  /// Signals
  void emitSelectedSpectraChanged(std::string const &spectra) { emit selectedSpectraChanged(spectra); }

  void emitSelectedSpectraChanged(IDA::WorkspaceIndex minimum, IDA::WorkspaceIndex maximum) {
    emit selectedSpectraChanged(minimum, maximum);
  }

  void emitMaskSpectrumChanged(IDA::WorkspaceIndex spectrum) { emit maskSpectrumChanged(spectrum); }

  /// Public methods
  MOCK_CONST_METHOD0(minimumSpectrum, IDA::WorkspaceIndex());
  MOCK_CONST_METHOD0(maximumSpectrum, IDA::WorkspaceIndex());

  MOCK_CONST_METHOD0(spectraString, std::string());
  MOCK_CONST_METHOD0(maskString, std::string());

  MOCK_METHOD1(displaySpectra, void(std::string const &spectraString));
  MOCK_METHOD1(displaySpectra, void(std::pair<IDA::WorkspaceIndex, IDA::WorkspaceIndex>));

  MOCK_METHOD2(setSpectraRange, void(IDA::WorkspaceIndex minimum, IDA::WorkspaceIndex maximum));

  MOCK_METHOD0(showSpectraErrorLabel, void());
  MOCK_METHOD0(hideSpectraErrorLabel, void());

  MOCK_METHOD1(setMaskSelectionEnabled, void(bool enabled));
  MOCK_METHOD0(clear, void());

  /// Public slots
  MOCK_METHOD1(setMinimumSpectrum, void(IDA::WorkspaceIndex spectrum));
  MOCK_METHOD1(setMaximumSpectrum, void(IDA::WorkspaceIndex spectrum));

  MOCK_METHOD1(setSpectraString, void(std::string const &spectraString));
  MOCK_METHOD1(setMaskString, void(std::string const &maskString));
};

/// Note that there is limited (if any) interaction going from this model to the
/// IndirectSpectrumSelectionView, meaning that not many methods are
/// required for mocking.
class MockIndirectSpectrumSelectionModel : public IndirectFittingModel {
public:
  /// Public methods
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(TableDatasetIndex dataIndex, IDA::WorkspaceIndex index));
  MOCK_CONST_METHOD0(isMultiFit, bool());

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(TableDatasetIndex index, IDA::WorkspaceIndex spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectSpectrumSelectionPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectSpectrumSelectionPresenterTest() { FrameworkManager::Instance(); }

  static IndirectSpectrumSelectionPresenterTest *createSuite() { return new IndirectSpectrumSelectionPresenterTest(); }

  static void destroySuite(IndirectSpectrumSelectionPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockIndirectSpectrumSelectionView>>();
    m_model = std::make_unique<NiceMock<MockIndirectSpectrumSelectionModel>>();
    m_presenter =
        std::make_unique<IndirectSpectrumSelectionPresenter>(std::move(m_model.get()), std::move(m_view.get()));

    SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(10));
    m_model->addWorkspace("WorkspaceName");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    m_presenter.reset(); /// The views destructor is called at this point
    m_model.reset();
    m_view.release();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_model_and_view() {
    std::string const excludeRegion("0-1");

    ON_CALL(*m_model, getExcludeRegion(TableDatasetIndex(0), IDA::WorkspaceIndex(0)))
        .WillByDefault(Return(excludeRegion));

    Expectation getMask = EXPECT_CALL(*m_model, getExcludeRegion(TableDatasetIndex(0), IDA::WorkspaceIndex(0)))
                              .Times(1)
                              .WillOnce(Return(excludeRegion));
    EXPECT_CALL(*m_view, setMaskString(excludeRegion)).Times(1).After(getMask);

    m_presenter->displayBinMask();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals (only the view emits signals here)
  ///----------------------------------------------------------------------

  void
  test_that_the_selectedSpectraChanged_signal_will_update_the_relevant_view_widgets_when_the_index_provided_is_in_range() {
    EXPECT_CALL(*m_view, hideSpectraErrorLabel()).Times(1);
    EXPECT_CALL(*m_view, setMaskSelectionEnabled(true)).Times(1);

    m_view->emitSelectedSpectraChanged("5");
  }

  void
  test_that_the_selectedSpectraChanged_signal_will_display_an_error_label_when_the_index_provided_is_out_of_range() {
    EXPECT_CALL(*m_view, showSpectraErrorLabel()).Times(1);
    EXPECT_CALL(*m_view, setMaskSelectionEnabled(false)).Times(1);

    m_view->emitSelectedSpectraChanged("11");
  }

  void
  test_that_the_selectedSpectraChanged_signal_will_not_display_an_error_label_when_the_range_provided_is_in_range() {
    EXPECT_CALL(*m_view, hideSpectraErrorLabel()).Times(1);
    EXPECT_CALL(*m_view, setMaskSelectionEnabled(true)).Times(1);

    m_view->emitSelectedSpectraChanged(0, 2);
  }

  void
  test_that_the_selectedSpectraChanged_signal_will_display_an_error_label_when_the_range_provided_is_out_of_range() {
    EXPECT_CALL(*m_view, showSpectraErrorLabel()).Times(1);
    EXPECT_CALL(*m_view, setMaskSelectionEnabled(false)).Times(1);

    m_view->emitSelectedSpectraChanged(0, 11);
  }

  void test_that_the_maskSpectrumChanged_signal_will_change_the_mask_by_calling_displayBinMask() {
    IDA::WorkspaceIndex const maskSpectrum(0);

    Expectation getMask =
        EXPECT_CALL(*m_model, getExcludeRegion(TableDatasetIndex(0), maskSpectrum)).Times(1).WillOnce(Return("0"));
    EXPECT_CALL(*m_view, setMaskString("0")).Times(1).After(getMask);

    m_view->emitMaskSpectrumChanged(maskSpectrum);
  }

  void
  test_that_the_maskSpectrumChanged_signal_will_change_the_mask_to_an_empty_string_if_the_index_provided_is_out_of_range() {
    IDA::WorkspaceIndex const maskSpectrum(11);

    Expectation getMask =
        EXPECT_CALL(*m_model, getExcludeRegion(TableDatasetIndex(0), maskSpectrum)).Times(1).WillOnce(Return(""));
    EXPECT_CALL(*m_view, setMaskString("")).Times(1).After(getMask);

    m_view->emitMaskSpectrumChanged(maskSpectrum);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods of the presenter
  ///----------------------------------------------------------------------

  void test_setActiveModelIndex_updates_spectra_with_new_index() {
    auto index = TableDatasetIndex{1};
    SetUpADSWithWorkspace ads("WorkspaceName2", createWorkspace(10));
    m_model->addWorkspace("WorkspaceName2");
    auto spectra = m_model->getSpectra(index);
    EXPECT_CALL(*m_view, setSpectraRange(spectra.front(), spectra.back())).Times(1);
    m_presenter->setActiveModelIndex(index);
  }

  void test_setActiveIndexToZero_updates_spectra_with_index_zero() {
    auto index = TableDatasetIndex{0};
    SetUpADSWithWorkspace ads("WorkspaceName2", createWorkspace(10));
    m_model->addWorkspace("WorkspaceName2");
    auto spectra = m_model->getSpectra(index);
    EXPECT_CALL(*m_view, setSpectraRange(spectra.front(), spectra.back())).Times(1);
    m_presenter->setActiveIndexToZero();
  }

private:
  std::unique_ptr<MockIndirectSpectrumSelectionView> m_view;
  std::unique_ptr<MockIndirectSpectrumSelectionModel> m_model;
  std::unique_ptr<IndirectSpectrumSelectionPresenter> m_presenter;
};
