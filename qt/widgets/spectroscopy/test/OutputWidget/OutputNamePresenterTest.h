// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidFrameworkTestHelpers/IndirectFitDataCreationHelper.h"
#include "MantidQtWidgets/Spectroscopy/MockObjects.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNamePresenter.h"

using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

class OutputNamePresenterTest : public CxxTest::TestSuite {
public:
  static OutputNamePresenterTest *createSuite() { return new OutputNamePresenterTest(); }

  static void destroySuite(OutputNamePresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockOutputNameView>>();
    auto model = std::make_unique<NiceMock<MockOutputNameModel>>();
    m_model = model.get();

    m_presenter = std::make_unique<OutputNamePresenter>(std::move(model), m_view.get());
    m_workspace = createWorkspace(2);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("test_red", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_model));
    m_presenter.reset();
    m_view.reset();
  }

  void test_label_warning_message_if_workspace_exists_on_ads() {
    auto textColor = "color: darkRed";
    auto text = "Output Name is in use, workspace will be overriden.";

    ON_CALL(*m_view, getCurrentOutputName()).WillByDefault(Return("test_red"));
    EXPECT_CALL(*m_view, setWarningLabel(text, textColor)).Times(1);
    m_presenter->generateWarningLabel();
  }

  void test_get_current_label() {
    EXPECT_CALL(*m_view, getCurrentLabel()).Times(1);
    m_presenter->getCurrentLabel();
  }

  void test_hide_output_name_box() {
    EXPECT_CALL(*m_view, hideOutputNameBox()).Times(1);
    m_presenter->hideOutputNameBox();
  }

  void test_label_warning_message_if_workspace_does_not_exists_on_ads() {
    auto textColor = "color: darkGreen";
    auto text = "Unused name, new workspace will be created";

    ON_CALL(*m_view, getCurrentOutputName()).WillByDefault(Return("workspace_red"));
    EXPECT_CALL(*m_view, setWarningLabel(text, textColor)).Times(1);
    m_presenter->generateWarningLabel();
  }

  void test_output_basename_set_adds_label_at_end_if_no_ws_suffix() {
    std::string const basename = "workspace_test";
    std::string const predictedOutput = basename + "_label";

    EXPECT_CALL(*m_view, enableLabelEditor()).Times(1);
    EXPECT_CALL(*m_model, setOutputBasename(basename)).Times(1);
    EXPECT_CALL(*m_view, setOutputNameLabel(predictedOutput)).Times(1);

    ON_CALL(*m_model, outputBasename()).WillByDefault(Return(basename));
    ON_CALL(*m_model, findIndexToInsertLabel(basename)).WillByDefault(Return(14));
    ON_CALL(*m_view, getCurrentLabel()).WillByDefault(Return("label"));

    m_presenter->setOutputWsBasename(basename);
  }

  void test_output_basename_set_adds_label_at_before_ws_suffix() {
    std::vector<std::string> test_suffices({"_red", "_sqw"});
    std::string const basename = "workspace_test_red";
    std::string const predictedOutput = "workspace_test_label_red";

    EXPECT_CALL(*m_view, enableLabelEditor()).Times(1);
    EXPECT_CALL(*m_model, setOutputBasename(basename)).Times(1);
    EXPECT_CALL(*m_view, setOutputNameLabel(predictedOutput)).Times(1);

    ON_CALL(*m_model, outputBasename()).WillByDefault(Return(basename));
    ON_CALL(*m_model, findIndexToInsertLabel(basename)).WillByDefault(Return(14));
    ON_CALL(*m_view, getCurrentLabel()).WillByDefault(Return("label"));

    m_presenter->setOutputWsBasename(basename, "_elwin");
  }

private:
  std::unique_ptr<NiceMock<MockOutputNameView>> m_view;
  NiceMock<MockOutputNameModel> *m_model;
  std::unique_ptr<OutputNamePresenter> m_presenter;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  MatrixWorkspace_sptr m_workspace;
};
