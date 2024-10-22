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
#include "MantidFrameworkTestHelpers/MockAlgorithm.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
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

    m_algorithm = std::make_shared<MockAlgorithm>();
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();

    TS_ASSERT(Mock::VerifyAndClearExpectations(m_view.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_algorithmRunner));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_outputPlotView.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_runView.get()));
    TS_ASSERT(Mock::VerifyAndClearExpectations(m_algorithm.get()));

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

  void test_runComplete_when_error_is_false() {
    m_model->setInputWorkspace("workspace_name");

    MatrixWorkspace_sptr workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
    auto const workspaceProperty =
        std::make_unique<PropertyWithValue<MatrixWorkspace_sptr>>("OutputWorkspace", workspace);

    m_algorithm->expectGetProperty("OutputWorkspace", workspaceProperty.get());

    EXPECT_CALL(*m_view, plotOutput(workspace)).Times(1);
    EXPECT_CALL(*m_model, getOutputWorkspace()).WillOnce(Return("workspace_name_Moments"));

    m_presenter->runComplete(m_algorithm, false);
  }

  void test_runComplete_when_error_is_true() {
    // Assert these are never called
    EXPECT_CALL(*m_view, plotOutput(_)).Times(0);
    EXPECT_CALL(*m_model, getOutputWorkspace()).Times(0);

    m_presenter->runComplete(m_algorithm, true);
  }

  void test_runComplete_when_error_is_false_and_the_workspace_has_fewer_than_five_histograms() {
    MatrixWorkspace_sptr workspace = WorkspaceCreationHelper::create2DWorkspace(4, 4);
    auto const workspaceProperty =
        std::make_unique<PropertyWithValue<MatrixWorkspace_sptr>>("OutputWorkspace", workspace);

    m_algorithm->expectGetProperty("OutputWorkspace", workspaceProperty.get());

    // Assert these are never called
    EXPECT_CALL(*m_view, plotOutput(workspace)).Times(0);
    EXPECT_CALL(*m_model, getOutputWorkspace()).Times(0);

    m_presenter->runComplete(m_algorithm, false);
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
  std::shared_ptr<MockAlgorithm> m_algorithm;
};
