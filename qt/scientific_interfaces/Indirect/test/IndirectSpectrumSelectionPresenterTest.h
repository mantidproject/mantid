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

#include "IndirectSpectrumSelectionPresenter.h"

#include "MantidAPI/IAlgorithm.h"
#include "MantidAPI/IFunction.h"
#include "MantidAPI/MatrixWorkspace.h"

using namespace Mantid::API;
using namespace MantidQt::CustomInterfaces::IDA;
using namespace testing;

namespace {

class MockIndirectSpectrumSelectionView : public IndirectSpectrumSelectionView {
public:
  void modifySelectedSpectra(std::string const &spectra) {
    emit selectedSpectraChanged(spectra);
  }

  void modifySelectedSpectra(std::size_t const &minimum,
                             std::size_t const &maximum) {
    emit selectedSpectraChanged(minimum, maximum);
  }

  void modifyMaskSpectrum(int const &spectrum) {
    emit maskSpectrumChanged(spectrum);
  }

  void modifyMask(std::string const &mask) { emit maskChanged(mask); }
};

class MockIndirectFittingModel : public IndirectFittingModel {
public:
  MOCK_CONST_METHOD0(isMultiFit, bool());
  MOCK_CONST_METHOD0(isInvalidFunction, boost::optional<std::string>());
  MOCK_CONST_METHOD0(getFittingFunction, IFunction_sptr());

  MOCK_METHOD2(addWorkspace,
               void(MatrixWorkspace_sptr workspace, Spectra const &spectra));
  MOCK_METHOD1(removeWorkspace, void(std::size_t index));
  MOCK_METHOD1(setFitFunction, void(IFunction_sptr function));
  MOCK_METHOD1(addOutput, void(IAlgorithm_sptr fitAlgorithm));

  MOCK_CONST_METHOD0(getFittingAlgorithm, IAlgorithm_sptr());

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

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));

    delete m_presenter;
    delete m_model;
    delete m_view;
  }

  void test_test() {}

private:
  MockIndirectSpectrumSelectionView *m_view;
  MockIndirectFittingModel *m_model;
  IndirectSpectrumSelectionPresenter *m_presenter;
};

#endif
