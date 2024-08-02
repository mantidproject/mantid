// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidQtWidgets/Common/AddWorkspaceDialog.h"
#include "MantidQtWidgets/Spectroscopy/DataValidationHelper.h"
#include "Processor/MomentsModel.h"
#include "Processor/MomentsPresenter.h"
#include "Processor/MomentsView.h"

#include "../QENSFitting/MockObjects.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidKernel/WarningSuppressions.h"

#include <MantidQtWidgets/Common/MockAlgorithmRunner.h>

using namespace Mantid::API;
using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace testing;

class MomentsPresenterTest : public CxxTest::TestSuite {
public:
  static MomentsPresenterTest *createSuite() { return new MomentsPresenterTest(); }

  static void destroySuite(MomentsPresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockMomentsView>>();
    m_outputPlotView = std::make_unique<NiceMock<MockOutputPlotOptionsView>>();
    m_runView = std::make_unique<NiceMock<MockRunView>>();
    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();
    auto model = std::make_unique<NiceMock<MockMomentsModel>>();
    m_model = model.get();

    ON_CALL(*m_view, getPlotOptions()).WillByDefault(Return((m_outputPlotView.get())));
    ON_CALL(*m_view, getRunView()).WillByDefault(Return((m_runView.get())));
    m_presenter =
        std::make_unique<MomentsPresenter>(nullptr, std::move(algorithmRunner), m_view.get(), std::move(model));

    m_workspace = createWorkspace(5);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("workspace_test", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_algorithmRunner));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_outputPlotView.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runView.get()));

    m_presenter.reset();
    m_view.reset();
    m_outputPlotView.reset();
    m_runView.reset();
  }

  ///----------------------------------------------------------------------
  /// Unit Tests that test the signals, methods and slots of the presenter
  ///----------------------------------------------------------------------

  void test_handleScaleChanged_sets_correct_bool_property() {

    EXPECT_CALL(*m_model, setScale(true)).Times((Exactly(1)));
    m_presenter->handleScaleChanged(true);

    EXPECT_CALL(*m_model, setScaleValue(true)).Times((Exactly(1)));
    m_presenter->handleScaleValueChanged(true);
  }

  void test_handleValueChanged_sets_correct_double_property() {
    double value = 0.1;

    EXPECT_CALL(*m_model, setEMin(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("EMin", value);
    EXPECT_CALL(*m_model, setEMax(value)).Times((Exactly(1)));
    m_presenter->handleValueChanged("EMax", value);
  }

private:
  NiceMock<MockMomentsModel> *m_model;
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  std::unique_ptr<NiceMock<MockOutputPlotOptionsView>> m_outputPlotView;
  std::unique_ptr<NiceMock<MockRunView>> m_runView;
  std::unique_ptr<NiceMock<MockMomentsView>> m_view;
  std::unique_ptr<MomentsPresenter> m_presenter;

  MatrixWorkspace_sptr m_workspace;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
};
