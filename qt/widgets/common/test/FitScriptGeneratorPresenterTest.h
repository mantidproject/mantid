// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/FitScriptGeneratorPresenter.h"

#include <cxxtest/TestSuite.h>

#include <memory>

using namespace MantidQt::MantidWidgets;

class FitScriptGeneratorPresenterTest : public CxxTest::TestSuite {

public:
  static FitScriptGeneratorPresenterTest *createSuite() {
    return new FitScriptGeneratorPresenterTest;
  }
  static void destroySuite(FitScriptGeneratorPresenterTest *suite) {
    delete suite;
  }

  FitScriptGeneratorPresenterTest() {}

  void setUp() override {
    // m_view = std::make_unique<MockFitScriptGeneratorView>();
    // m_model = std::make_unique<MockFitScriptGeneratorModel>();
    // m_presenter = std::make_unique<FitScriptGeneratorPresenter>(m_view.get(),
    //                                                            m_model.get());
  }

  // void tearDown() override { m_presenter.reset(); }

  void test_empty() {}

  // private:
  // std::unique_ptr<MockFitScriptGeneratorView> m_view;
  // std::unique_ptr<MockFitScriptGeneratorModel> m_model;
  // std::unique_ptr<FitScriptGeneratorPresenter> m_presenter;
};
