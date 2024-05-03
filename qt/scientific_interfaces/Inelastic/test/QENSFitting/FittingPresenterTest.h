// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"
#include "MockObjects.h"
#include "QENSFitting/FittingPresenter.h"

#include <memory>

using namespace testing;

class FittingPresenterTest : public CxxTest::TestSuite {
public:
  static FittingPresenterTest *createSuite() { return new FittingPresenterTest(); }

  static void destroySuite(FittingPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_tab = std::make_unique<NiceMock<MockFitTab>>();
    auto model = std::make_unique<NiceMock<MockFittingModel>>();
    m_model = model.get();
    m_browser = std::make_unique<NiceMock<MockInelasticFitPropertyBrowser>>();
    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();
    m_presenter =
        std::make_unique<FittingPresenter>(m_tab.get(), m_browser.get(), std::move(model), std::move(algorithmRunner));
  }

  void test_1() {}

private:
  std::unique_ptr<NiceMock<MockFitTab>> m_tab;
  NiceMock<MockFittingModel> *m_model;
  std::unique_ptr<NiceMock<MockInelasticFitPropertyBrowser>> m_browser;
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  std::unique_ptr<FittingPresenter> m_presenter;
};
