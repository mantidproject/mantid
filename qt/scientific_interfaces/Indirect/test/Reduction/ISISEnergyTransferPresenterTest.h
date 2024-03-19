// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "../../../Inelastic/test/Common/MockObjects.h"
#include "MockObjects.h"
#include "Reduction/ISISEnergyTransferPresenter.h"

#include "MantidAPI/AlgorithmProperties.h"
#include "MantidKernel/WarningSuppressions.h"

using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {

std::unique_ptr<Mantid::API::AlgorithmRuntimeProps> defaultGroupingProps() {
  auto properties = std::make_unique<Mantid::API::AlgorithmRuntimeProps>();
  Mantid::API::AlgorithmProperties::update("GroupingMethod", std::string("IPF"), *properties);
  return properties;
}

} // namespace

class IETPresenterTest : public CxxTest::TestSuite {
public:
  static IETPresenterTest *createSuite() { return new IETPresenterTest(); }
  static void destroySuite(IETPresenterTest *suite) { delete suite; }

  void setUp() override {
    auto model = std::make_unique<NiceMock<MockIETModel>>();

    m_outputOptionsView = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();

    m_view = std::make_unique<NiceMock<MockIETView>>();
    ON_CALL(*m_view, getPlotOptionsView()).WillByDefault(Return(m_outputOptionsView.get()));

    m_model = model.get();
    m_idrUI = std::make_unique<NiceMock<MockIndirectDataReduction>>();

    m_presenter = std::make_unique<IETPresenter>(m_idrUI.get(), m_view.get(), std::move(model));
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_idrUI));

    m_presenter.reset();
    m_idrUI.reset();
    m_view.reset();
  }

  void test_fetch_instrument_data() {
    // ON_CALL(*m_model, runIETAlgorithm(_, _, _)).WillByDefault(Return(""));
    // ON_CALL(*m_view, getRunData());

    // ExpectationSet expectRunData = EXPECT_CALL(*m_view, getRunData()).Times(1);
    // ExpectationSet expectRunAlgo = EXPECT_CALL(*m_model, runIETAlgorithm(_, _, _)).Times(1).After(expectRunData);

    // m_presenter->run();
  }

private:
  std::unique_ptr<IETPresenter> m_presenter;

  std::unique_ptr<NiceMock<MockIETView>> m_view;
  NiceMock<MockIETModel> *m_model;
  std::unique_ptr<NiceMock<MockIndirectDataReduction>> m_idrUI;

  std::unique_ptr<NiceMock<MockOutputPlotOptionsView>> m_outputOptionsView;
};