// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "BayesFitting/StretchPresenter.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidQtWidgets/Common/MockAlgorithmRunner.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "MockObjects.h"

#include <memory>

using namespace MantidQt::CustomInterfaces;
using namespace testing;

namespace {
auto &ads = Mantid::API::AnalysisDataService::Instance();
}

class StretchPresenterTest : public CxxTest::TestSuite {
public:
  static StretchPresenterTest *createSuite() { return new StretchPresenterTest(); }
  static void destroySuite(StretchPresenterTest *suite) { delete suite; }

  void setUp() override {
    auto algorithmRunner = std::make_unique<NiceMock<MockAlgorithmRunner>>();
    m_algorithmRunner = algorithmRunner.get();
    auto model = std::make_unique<NiceMock<MockStretchModel>>();
    m_model = model.get();
    m_view = std::make_unique<NiceMock<MockStretchView>>();
    m_runView = std::make_unique<NiceMock<MockRunView>>();

    ON_CALL(*m_view, getRunWidget()).WillByDefault(Return(m_runView.get()));
    m_presenter =
        std::make_unique<StretchPresenter>(nullptr, m_view.get(), std::move(model), std::move(algorithmRunner));

    m_workspace = WorkspaceCreationHelper::create2DWorkspace(5, 4);
  }

  void tearDown() override {
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_algorithmRunner));

    m_presenter.reset();
    m_view.reset();
    ads.clear();
  }

  void test_handleValidation_invalid_input() {
    EXPECT_CALL(*m_view, validateUserInput(_)).WillOnce(Invoke([](IUserInputValidator *validator) {
      validator->addErrorMessage("Invalid input");
    }));

    auto validator = std::make_unique<UserInputValidator>();
    m_presenter->handleValidation(validator.get());

    TS_ASSERT(!validator->isAllInputValid());
  }

  void test_handleValidation_valid_input() {
    EXPECT_CALL(*m_view, validateUserInput(_)).WillOnce(Invoke([](IUserInputValidator *validator) {
      (void)validator;
    }));

    auto validator = std::make_unique<UserInputValidator>();
    m_presenter->handleValidation(validator.get());

    TS_ASSERT(validator->isAllInputValid());
  }

  void test_handleRun_with_Empty_savedir_and_user_rejects_prompt() {
    auto &configSvc = Mantid::Kernel::ConfigService::Instance();
    configSvc.setString("defaultsave.directory", "");
    ON_CALL(*m_view, displaySaveDirectoryMessage()).WillByDefault(Return(true));

    EXPECT_CALL(*m_view, displaySaveDirectoryMessage()).Times(1);

    m_presenter->handleRun();
  }

  void test_handleRun_with_Empty_savedir_and_user_enter_savedir() {
    auto &configSvc = Mantid::Kernel::ConfigService::Instance();
    configSvc.setString("defaultsave.directory", "/test/test");
    ON_CALL(*m_view, displaySaveDirectoryMessage()).WillByDefault(Return(false));

    StretchRunData runData("sample_ws", "res_ws", -0.5, 0.5, 50, true, "flat", 30, 1, true);

    ON_CALL(*m_view, getRunData()).WillByDefault(Return(runData));

    EXPECT_CALL(*m_view, setPlotADSEnabled(false)).Times(1);

    EXPECT_CALL(*m_model, stretchAlgorithm(_, _, _, _)).Times(1);

    m_presenter->handleRun();
  }

  void test_handleRun_with_valid_input_and_savedir() {
    StretchRunData runData("sample_ws", "res_ws", -0.5, 0.5, 50, true, "flat", 30, 1, true);

    ON_CALL(*m_view, getRunData()).WillByDefault(Return(runData));
    EXPECT_CALL(*m_view, setPlotADSEnabled(false)).Times(1);

    EXPECT_CALL(*m_model, stretchAlgorithm(_, _, _, _)).Times(1);

    m_presenter->handleRun();
  }

  void test_notifySaveClicked_with_output_workspaces() {
    StretchRunData runData("sample_ws", "res_ws", -0.5, 0.5, 50, true, "flat", 30, 1, true);

    auto const cutIndex = runData.sampleName.find_last_of("_");
    auto const baseName = runData.sampleName.substr(0, cutIndex);
    auto fitWorkspaceName = baseName + "_Stretch_Fit";
    auto contourWorkspaceName = baseName + "_Stretch_Contour";

    ads.addOrReplace(fitWorkspaceName, m_workspace);
    ads.addOrReplace(contourWorkspaceName, m_workspace);

    ON_CALL(*m_view, getRunData()).WillByDefault(Return(runData));
    EXPECT_CALL(*m_view, setPlotADSEnabled(false)).Times(1);

    EXPECT_CALL(*m_model, stretchAlgorithm(_, _, _, _)).Times(1);

    m_presenter->handleRun();

    EXPECT_CALL(*m_model, setupSaveAlgorithm(_)).Times(2);

    EXPECT_CALL(*m_algorithmRunner,
                execute(Matcher<std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>>(SizeIs(Eq(2)))))
        .Times(1);

    m_presenter->notifySaveClicked();
  }

private:
  NiceMock<MockAlgorithmRunner> *m_algorithmRunner;
  NiceMock<MockStretchModel> *m_model;
  std::unique_ptr<NiceMock<MockRunView>> m_runView;
  std::unique_ptr<NiceMock<MockStretchView>> m_view;
  std::unique_ptr<StretchPresenter> m_presenter;
  Mantid::API::MatrixWorkspace_sptr m_workspace;
};
