// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTSPECTRUMSELECTIONPRESENTERTEST_H_
#define MANTIDQT_INDIRECTSPECTRUMSELECTIONPRESENTERTEST_H_

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

GNU_DIAG_OFF_SUGGEST_OVERRIDE

/// Mock object to mock the view
class MockIndirectSpectrumSelectionView : public IndirectSpectrumSelectionView {
public:
  /// Signals
  void emitSelectedSpectraChanged(std::string const &spectra) {
    emit selectedSpectraChanged(spectra);
  }

  void emitSelectedSpectraChanged(std::size_t minimum, std::size_t maximum) {
    emit selectedSpectraChanged(minimum, maximum);
  }

  void emitMaskSpectrumChanged(int spectrum) {
    emit maskSpectrumChanged(spectrum);
  }

  /// Public methods
  MOCK_CONST_METHOD0(minimumSpectrum, std::size_t());
  MOCK_CONST_METHOD0(maximumSpectrum, std::size_t());

  MOCK_CONST_METHOD0(spectraString, std::string());
  MOCK_CONST_METHOD0(maskString, std::string());

  MOCK_METHOD1(displaySpectra, void(std::string const &spectraString));
  MOCK_METHOD2(displaySpectra, void(int minimum, int maximum));

  MOCK_METHOD2(setSpectraRange, void(int minimum, int maximum));

  MOCK_METHOD0(showSpectraErrorLabel, void());
  MOCK_METHOD0(hideSpectraErrorLabel, void());

  MOCK_METHOD1(setMaskSelectionEnabled, void(bool enabled));
  MOCK_METHOD0(clear, void());

  /// Public slots
  MOCK_METHOD1(setMinimumSpectrum, void(std::size_t spectrum));
  MOCK_METHOD1(setMaximumSpectrum, void(std::size_t spectrum));

  MOCK_METHOD1(setSpectraString, void(std::string const &spectraString));
  MOCK_METHOD1(setMaskString, void(std::string const &maskString));
};

/// Note that there is limited (if any) interaction going from this model to the
/// IndirectSpectrumSelectionView, meaning that not many methods are required
/// for mocking.
class MockIndirectSpectrumSelectionModel : public IndirectFittingModel {
public:
  /// Public methods
  MOCK_CONST_METHOD2(getExcludeRegion,
                     std::string(std::size_t dataIndex, std::size_t index));
  MOCK_CONST_METHOD0(isMultiFit, bool());

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };

  std::vector<std::string> getSpectrumDependentAttributes() const override {
    return {};
  };
};

GNU_DIAG_ON_SUGGEST_OVERRIDE

class IndirectSpectrumSelectionPresenterTest : public CxxTest::TestSuite {
public:
  /// Needed to make sure everything is initialized
  IndirectSpectrumSelectionPresenterTest() { FrameworkManager::Instance(); }

  static IndirectSpectrumSelectionPresenterTest *createSuite() {
    return new IndirectSpectrumSelectionPresenterTest();
  }

  static void destroySuite(IndirectSpectrumSelectionPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockIndirectSpectrumSelectionView>>();
    m_model = std::make_unique<NiceMock<MockIndirectSpectrumSelectionModel>>();
    m_presenter = std::make_unique<IndirectSpectrumSelectionPresenter>(
        std::move(m_model.get()), std::move(m_view.get()));

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

  void test_that_the_model_and_view_have_been_instantiated_correctly() {
    std::size_t const maxSpectrum(3);

    ON_CALL(*m_view, maximumSpectrum()).WillByDefault(Return(maxSpectrum));
    ON_CALL(*m_model, isMultiFit()).WillByDefault(Return(false));

    EXPECT_CALL(*m_view, maximumSpectrum())
        .Times(1)
        .WillOnce(Return(maxSpectrum));
    EXPECT_CALL(*m_model, isMultiFit()).Times(1).WillOnce(Return(false));

    m_view->maximumSpectrum();
    m_model->isMultiFit();
  }

  void
  test_that_invoking_a_presenter_method_will_call_the_relevant_methods_in_the_model_and_view() {
    std::string const excludeRegion("0-1");

    ON_CALL(*m_model, getExcludeRegion(0, 0))
        .WillByDefault(Return(excludeRegion));

    Expectation getMask = EXPECT_CALL(*m_model, getExcludeRegion(0, 0))
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

  void
  test_that_the_maskSpectrumChanged_signal_will_change_the_mask_by_calling_displayBinMask() {
    std::size_t const maskSpectrum(0);

    Expectation getMask =
        EXPECT_CALL(*m_model, getExcludeRegion(0, maskSpectrum))
            .Times(1)
            .WillOnce(Return("0"));
    EXPECT_CALL(*m_view, setMaskString("0")).Times(1).After(getMask);

    m_view->emitMaskSpectrumChanged(maskSpectrum);
  }

  void
  test_that_the_maskSpectrumChanged_signal_will_change_the_mask_to_an_empty_string_if_the_index_provided_is_out_of_range() {
    std::size_t const maskSpectrum(11);

    Expectation getMask =
        EXPECT_CALL(*m_model, getExcludeRegion(0, maskSpectrum))
            .Times(1)
            .WillOnce(Return(""));
    EXPECT_CALL(*m_view, setMaskString("")).Times(1).After(getMask);

    m_view->emitMaskSpectrumChanged(maskSpectrum);
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the methods and slots of the view
  ///----------------------------------------------------------------------

  void
  test_that_minimumSpectrum_returns_the_spectrum_number_that_it_is_set_as() {
    std::size_t const minSpectrum(3);

    EXPECT_CALL(*m_view, setMinimumSpectrum(minSpectrum)).Times(1);
    EXPECT_CALL(*m_view, minimumSpectrum())
        .Times(1)
        .WillOnce(Return(minSpectrum));

    m_view->setMinimumSpectrum(minSpectrum);
    m_view->minimumSpectrum();
  }

  void
  test_that_maximumSpectrum_returns_the_spectrum_number_that_it_is_set_as() {
    std::size_t const maxSpectrum(3);

    EXPECT_CALL(*m_view, setMaximumSpectrum(maxSpectrum)).Times(1);
    EXPECT_CALL(*m_view, maximumSpectrum())
        .Times(1)
        .WillOnce(Return(maxSpectrum));

    m_view->setMaximumSpectrum(maxSpectrum);
    m_view->maximumSpectrum();
  }

  void test_that_spectraString_returns_the_string_which_has_been_set() {
    std::string const spectra("2,4-5");

    EXPECT_CALL(*m_view, setSpectraString(spectra)).Times(1);
    EXPECT_CALL(*m_view, spectraString()).Times(1).WillOnce(Return(spectra));

    m_view->setSpectraString(spectra);
    m_view->spectraString();
  }

  void test_that_maskString_returns_the_string_which_has_been_set() {
    std::string const mask("2,4-5");

    EXPECT_CALL(*m_view, setMaskString(mask)).Times(1);
    EXPECT_CALL(*m_view, maskString()).Times(1).WillOnce(Return(mask));

    m_view->setMaskString(mask);
    m_view->maskString();
  }

  void
  test_that_displaySpectra_will_change_the_spectraString_to_the_string_provided() {
    std::string const spectra("2,4-5");

    EXPECT_CALL(*m_view, displaySpectra(spectra)).Times(1);
    EXPECT_CALL(*m_view, spectraString()).Times(1).WillOnce(Return(spectra));

    m_view->displaySpectra(spectra);
    m_view->spectraString();
  }

  void
  test_that_displaySpectra_will_set_the_minimum_and_maximum_of_the_spectraString() {
    int const minSpectrum(2);
    int const maxSpectrum(5);

    EXPECT_CALL(*m_view, displaySpectra(minSpectrum, maxSpectrum)).Times(1);
    EXPECT_CALL(*m_view, spectraString()).Times(1).WillOnce(Return("2-5"));

    m_view->displaySpectra(minSpectrum, maxSpectrum);
    m_view->spectraString();
  }

  void test_that_setSpectraRange_will_set_the_minimum_and_maximum_spectrums() {
    int const minSpectrum(2);
    int const maxSpectrum(5);

    EXPECT_CALL(*m_view, setSpectraRange(minSpectrum, maxSpectrum)).Times(1);
    EXPECT_CALL(*m_view, minimumSpectrum()).Times(1).WillOnce(Return(2));
    EXPECT_CALL(*m_view, maximumSpectrum()).Times(1).WillOnce(Return(2));

    m_view->setSpectraRange(minSpectrum, maxSpectrum);
    m_view->minimumSpectrum();
    m_view->maximumSpectrum();
  }

  void test_that_clear_will_empty_the_spectraString_and_maskString() {
    m_view->setSpectraString("2-5");
    m_view->setMaskString("7-8");

    EXPECT_CALL(*m_view, clear()).Times(1);
    EXPECT_CALL(*m_view, spectraString()).Times(1).WillOnce(Return(""));
    EXPECT_CALL(*m_view, maskString()).Times(1).WillOnce(Return(""));

    m_view->clear();
    m_view->spectraString();
    m_view->maskString();
  }

  void test_that_clear_will_set_the_minimum_and_maximum_spectrums_to_be_zero() {
    m_view->setMinimumSpectrum(2);
    m_view->setMaximumSpectrum(4);

    EXPECT_CALL(*m_view, clear()).Times(1);
    EXPECT_CALL(*m_view, minimumSpectrum()).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*m_view, maximumSpectrum()).Times(1).WillOnce(Return(0));

    m_view->clear();
    m_view->minimumSpectrum();
    m_view->maximumSpectrum();
  }

private:
  std::unique_ptr<MockIndirectSpectrumSelectionView> m_view;
  std::unique_ptr<MockIndirectSpectrumSelectionModel> m_model;
  std::unique_ptr<IndirectSpectrumSelectionPresenter> m_presenter;
};

#endif
