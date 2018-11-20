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

#include "../General/UserInputValidator.h"
#include "IndirectFitData.h"
#include "IndirectFittingModel.h"
#include "IndirectSpectrumSelectionPresenter.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidTestHelpers/IndirectFitDataCreationHelper.h"

#include <boost/variant.hpp>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

namespace {

/**
 * QApplication
 *
 * Uses setUpWorld/tearDownWorld to initialize & finalize
 * QApplication object
 */
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

  void emitMaskChanged(std::string const &mask) { emit maskChanged(mask); }

  /// Public methods
  MOCK_CONST_METHOD0(minimumSpectrum, std::size_t());
  MOCK_CONST_METHOD0(maximumSpectrum, std::size_t());

  MOCK_CONST_METHOD0(spectraString, std::string());
  MOCK_CONST_METHOD0(maskString, std::string());

  MOCK_METHOD1(displaySpectra, void(std::string const &spectraString));
  MOCK_METHOD2(displaySpectra, void(int minimum, int maximum));

  MOCK_METHOD2(setSpectraRange, void(int minimum, int maximum));

  MOCK_METHOD1(setSpectraRegex, void(std::string const &regex));
  MOCK_METHOD1(setMaskBinsRegex, void(std::string const &regex));

  MOCK_CONST_METHOD1(validateSpectraString,
                     UserInputValidator &(UserInputValidator &uiv));
  MOCK_CONST_METHOD1(validateMaskBinsString,
                     UserInputValidator &(UserInputValidator &uiv));

  MOCK_METHOD0(showSpectraErrorLabel, void());
  MOCK_METHOD0(showMaskBinErrorLabel, void());
  MOCK_METHOD0(hideSpectraErrorLabel, void());
  MOCK_METHOD0(hideMaskBinErrorLabel, void());

  MOCK_METHOD1(setMaskSelectionEnabled, void(bool enabled));
  MOCK_METHOD0(clear, void());

  /// Public slots
  MOCK_METHOD1(setMinimumSpectrum, void(std::size_t spectrum));
  MOCK_METHOD1(setMaximumSpectrum, void(std::size_t spectrum));
  MOCK_METHOD1(setMaskSpectrum, void(std::size_t spectrum));

  MOCK_METHOD1(setSpectraString, void(std::string const &spectraString));
  MOCK_METHOD1(setMaskString, void(std::string const &maskString));
  MOCK_METHOD1(setMaskSpectraList,
               void(std::vector<std::size_t> const &maskString));

  MOCK_METHOD0(hideSpectrumSelector, void());
  MOCK_METHOD0(showSpectrumSelector, void());
  MOCK_METHOD0(hideMaskSpectrumSelector, void());
  MOCK_METHOD0(showMaskSpectrumSelector, void());

  MOCK_METHOD0(clearMaskString, void());
};

class MockIndirectFittingModel : public IndirectFittingModel {
public:
  /// Public methods
  // MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(std::size_t index));
  //// MOCK_CONST_METHOD1(getSpectra, Spectra(std::size_t index));
  // MOCK_CONST_METHOD2(getFittingRange,
  //                   std::pair<double, double>(std::size_t dataIndex,
  //                                             std::size_t spectrum));
  // MOCK_CONST_METHOD2(getExcludeRegion,
  //                   std::string(std::size_t dataIndex, std::size_t index));
  // MOCK_CONST_METHOD3(createDisplayName,
  //                   std::string(std::string const &formatString,
  //                               std::string const &rangeDelimiter,
  //                               std::size_t dataIndex));
  // MOCK_CONST_METHOD3(createOutputName,
  //                   std::string(std::string const &formatString,
  //                               std::string const &rangeDelimiter,
  //                               std::size_t dataIndex));
  MOCK_CONST_METHOD0(isMultiFit, bool());
  // MOCK_CONST_METHOD2(isPreviouslyFit,
  //                   bool(std::size_t dataIndex, std::size_t spectrum));
  // MOCK_CONST_METHOD1(hasZeroSpectra, bool(std::size_t dataIndex));
  MOCK_CONST_METHOD0(isInvalidFunction, boost::optional<std::string>());
  // MOCK_CONST_METHOD0(numberOfWorkspaces, std::size_t());
  // MOCK_CONST_METHOD1(getNumberOfSpectra, std::size_t(std::size_t index));
  // MOCK_CONST_METHOD0(getFitParameterNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(getFittingFunction, IFunction_sptr());

  //// MOCK_METHOD1(setFittingData, void(PrivateFittingData &&fittingData));
  // MOCK_METHOD2(setSpectra,
  //             void(std::string const &spectra, std::size_t dataIndex));
  //// MOCK_METHOD2(setSpectra, void(Spectra &&spectra, std::size_t dataIndex));
  MOCK_METHOD2(setSpectra,
               void(MantidQt::CustomInterfaces::IDA::Spectra const &spectra,
                    std::size_t dataIndex));
  // MOCK_METHOD3(setStartX, void(double startX, std::size_t dataIndex,
  //                             std::size_t spectrum));
  // MOCK_METHOD3(setEndX,
  //             void(double endX, std::size_t dataIndex, std::size_t
  //             spectrum));
  // MOCK_METHOD3(setExcludeRegion,
  //             void(std::string const &exclude, std::size_t dataIndex,
  //                  std::size_t spectrum));

  // MOCK_METHOD1(addWorkspace, void(std::string const &workspaceName));
  // MOCK_METHOD2(addWorkspace, void(std::string const &workspaceName,
  //                                std::string const &spectra));
  ///// MOCK_METHOD2(addWorkspace,
  /////            void(std::string const &workspaceName, Spectra const
  /////            &spectra));
  //// MOCK_METHOD2(addWorkspace,
  ////              void(MatrixWorkspace_sptr workspace, Spectra const
  ////              &spectra));
  MOCK_METHOD1(removeWorkspace, void(std::size_t index));
  //// MOCK_METHOD0(clearWorkspaces, PrivateFittingData());
  // MOCK_METHOD1(setFittingMode, void(FittingMode mode));
  MOCK_METHOD1(setFitFunction, void(IFunction_sptr function));
  // MOCK_METHOD3(setDefaultParameterValue,
  //             void(std::string const &name, double value,
  //                  std::size_t dataIndex));
  // MOCK_METHOD2(addSingleFitOutput,
  //             void(IAlgorithm_sptr fitAlgorithm, std::size_t index));
  MOCK_METHOD1(addOutput, void(IAlgorithm_sptr fitAlgorithm));

  MOCK_CONST_METHOD0(getFittingAlgorithm, IAlgorithm_sptr());

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override {
    UNUSED_ARG(index);
    UNUSED_ARG(spectrum);
    return "";
  };
};

} // namespace

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
    m_view = new NiceMock<MockIndirectSpectrumSelectionView>();
    m_model = new NiceMock<MockIndirectFittingModel>();
    m_presenter = new IndirectSpectrumSelectionPresenter(m_model, m_view);

    SetUpADSWithWorkspace ads("WorkspaceName", createWorkspace(10));
    m_model->addWorkspace("WorkspaceName");
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

    delete m_presenter;
    delete m_model;
    // delete m_view; causes error
  }

  void test_that_the_presenter_has_been_initialized() {
    /// Not using m_view and m_present, because they are already initialized
    /// after setUp()
    // MockIndirectSpectrumSelectionView view;
    // TS_ASSERT(view);
    // MockIndirectFittingModel model;
    // TS_ASSERT(model);
    // IndirectSpectrumSelectionPresenter presenter(&model, &view);
    // TS_ASSERT(presenter);
  }

  void test_test() {
    // ON_CALL(*m_view, minimumSpectrum()).WillByDefault(Return(1));
  }

  void
  test_that_the_selectedSpectraChanged_signal_will_set_the_spectra_in_the_model() {
	/// Expectations
    EXPECT_CALL(*m_view, hideSpectraErrorLabel()).Times(1);
    // EXPECT_CALL(*m_view, setMaskSelectionEnabled(true)).Times(1);
    Spectra const spectra = DiscontinuousSpectra<std::size_t>("5");

    EXPECT_CALL(*m_model, setSpectra(spectra, 0)).Times(1);
	/// Invoke expectations
    m_view->emitSelectedSpectraChanged("5");
  }

  void test_next() {}

private:
  MockIndirectSpectrumSelectionView *m_view;
  MockIndirectFittingModel *m_model;
  IndirectSpectrumSelectionPresenter *m_presenter;
};

#endif
