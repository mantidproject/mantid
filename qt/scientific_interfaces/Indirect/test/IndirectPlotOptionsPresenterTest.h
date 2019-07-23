// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTIDQT_INDIRECTPLOTOPTIONSPRESENTERTEST_H_
#define MANTIDQT_INDIRECTPLOTOPTIONSPRESENTERTEST_H_

#include <cxxtest/TestSuite.h>
//#include <gmock/gmock.h>

class IndirectPlotOptionsPresenterTest : public CxxTest::TestSuite {
public:
  static IndirectPlotOptionsPresenterTest *createSuite() {
    return new IndirectPlotOptionsPresenterTest();
  }

  static void destroySuite(IndirectPlotOptionsPresenterTest *suite) {
    delete suite;
  }

  void setUp() override {
    // m_view = std::make_unique<MockIndirectFitOutputOptionsView>();
    // m_model = std::make_unique<MockIndirectFitOutputOptionsModel>();

    // m_presenter = std::make_unique<IndirectFitOutputOptionsPresenter>(
    //    m_model.get(), m_view.get());
  }

  void tearDown() override {
    // TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    // TS_ASSERT(Mock::VerifyAndClearExpectations(m_model.get()));

    // m_view.reset();
    // m_presenter.reset(); /// The model is destructed by the presenter
    // m_model.release();
  }

  ///----------------------------------------------------------------------
  /// Unit tests to check for successful presenter instantiation
  ///----------------------------------------------------------------------

  void test_that_the_presenter_has_been_instantiated() {
    // TS_ASSERT(m_view);
    // TS_ASSERT(m_model);
    // TS_ASSERT(m_presenter);
  }
};

#endif /* MANTIDQT_INDIRECTPLOTOPTIONSPRESENTERTEST_H_ */
