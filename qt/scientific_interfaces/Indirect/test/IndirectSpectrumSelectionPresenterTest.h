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

using namespace MantidQt::CustomInterfaces::IDA;

class MockIndirectSpectrumSelectionView : public IndirectSpectrumSelectionView {
};

class MockIndirectFittingModel : public IndirectFittingModel {};

class IndirectSpectrumSelectionPresenterTest : public CxxTest::TestSuite {
public:
  static IndirectSpectrumSelectionPresenterTest *createSuite() {
    return new IndirectSpectrumSelectionPresenterTest();
  }

  static void destroySuite(IndirectSpectrumSelectionPresenterTest *suite) {
    delete suite;
  }

  void test_test() {}

private:
  MockIndirectSpectrumSelectionView *m_view;
  MockIndirectFittingModel *m_model;
  IndirectSpectrumSelectionPresenter *m_presenter;
};

#endif
