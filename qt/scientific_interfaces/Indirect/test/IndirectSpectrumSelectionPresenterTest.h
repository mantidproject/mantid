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
#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

namespace {

class MockIndirectSpectrumSelectionView
    : public IIndirectSpectrumSelectionView {
public:
  /// Signals
  void changeSelectedSpectra(std::string const &spectra) {
    emit selectedSpectraChanged(spectra);
  }

  void changeSelectedSpectra(std::size_t const &minimum,
                             std::size_t const &maximum) {
    emit selectedSpectraChanged(minimum, maximum);
  }

  void changeMaskedSpectrum(int const &spectrum) {
    emit maskSpectrumChanged(spectrum);
  }

  void changeMask(std::string const &mask) { emit maskChanged(mask); }

  /// Public methods
  MOCK_CONST_METHOD0(selectionMode, SpectrumSelectionMode());

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
  // public:
  //  MOCK_CONST_METHOD0(isMultiFit, bool());
  //  MOCK_CONST_METHOD0(isInvalidFunction, boost::optional<std::string>());
  //  MOCK_CONST_METHOD0(getFittingFunction, IFunction_sptr());
  //
  //  MOCK_METHOD2(addWorkspace,
  //               void(MatrixWorkspace_sptr workspace, Spectra const
  //               &spectra));
  //  MOCK_METHOD1(removeWorkspace, void(std::size_t index));
  //  MOCK_METHOD1(setFitFunction, void(IFunction_sptr function));
  //  MOCK_METHOD1(addOutput, void(IAlgorithm_sptr fitAlgorithm));
  //
  //  MOCK_CONST_METHOD0(getFittingAlgorithm, IAlgorithm_sptr());

private:
  std::string sequentialFitOutputName() const override { return ""; };
  std::string simultaneousFitOutputName() const override { return ""; };
  std::string singleFitOutputName(std::size_t index,
                                  std::size_t spectrum) const override {
    (void)index;
    (void)spectrum;
    return "";
  };
};

} // namespace

class IndirectSpectrumSelectionPresenterTest : public CxxTest::TestSuite {
public:
  /// To make sure everything is initialized
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
  }

  // void tearDown() override {
  //  TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
  //  TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

  //  delete m_presenter;
  //  delete m_model;
  //  delete m_view;
  //}

  // void test_initialize() {
  //  MockIndirectSpectrumSelectionView view;
  //  MockIndirectFittingModel model;
  //  IndirectSpectrumSelectionPresenter presenter(&model, &view);

  //  EXPECT_CALL(view, maximumSpectrum()).Times(0);
  //}

  void test_test() {}

private:
  MockIndirectSpectrumSelectionView *m_view;
  MockIndirectFittingModel *m_model;
  IndirectSpectrumSelectionPresenter *m_presenter;
};

#endif
