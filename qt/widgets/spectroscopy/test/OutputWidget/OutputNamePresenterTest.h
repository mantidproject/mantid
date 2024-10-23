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

#include "MantidKernel/WarningSuppressions.h"

using namespace Mantid::IndirectFitDataCreationHelper;
using namespace MantidQt::CustomInterfaces;
using namespace testing;

class OutputNamePresenterTest : public CxxTest::TestSuite {
public:
  static OutputNamePresenterTest *createSuite() { return new OutputNamePresenterTest(); }

  static void destroySuite(OutputNamePresenterTest *suite) { delete suite; }

  void setUp() override {
    m_view = std::make_unique<NiceMock<MockOutputNameView>>();

    m_presenter = std::make_unique<OutputNamePresenter>(m_view.get());
    m_workspace = createWorkspace(2);
    m_ads = std::make_unique<SetUpADSWithWorkspace>("test_red", m_workspace);
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    TS_ASSERT(Mock::VerifyAndClearExpectations(&m_view));
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

  void test_label_warning_message_if_workspace_does_not_exists_on_ads() {
    auto textColor = "color: darkGreen";
    auto text = "Unused name, new workspace will be created";

    ON_CALL(*m_view, getCurrentOutputName()).WillByDefault(Return("workspace_red"));
    EXPECT_CALL(*m_view, setWarningLabel(text, textColor)).Times(1);
    m_presenter->generateWarningLabel();
  }

  void test_index_label_position_is_correct() {
    std::vector<std::string> test_suffices({"_red", "_sqw"});
    m_presenter->setWsSuffixes(test_suffices);
    auto pos = m_presenter->findIndexToInsertLabel("test_red");
    TS_ASSERT_EQUALS(pos, 4);
    pos = m_presenter->findIndexToInsertLabel("test");
    TS_ASSERT_EQUALS(pos, 4);
    pos = m_presenter->findIndexToInsertLabel("test_red_sqw");
    TS_ASSERT_EQUALS(pos, 8);
  }

  void test_output_basename_set_adds_label_at_end_if_no_ws_suffix() {
    EXPECT_CALL(*m_view, enableLabelEditor()).Times(1);
    ON_CALL(*m_view, getCurrentLabel()).WillByDefault(Return("label"));
    EXPECT_CALL(*m_view, setOutputNameLabel("workspace_test_label")).Times(1);
    m_presenter->setOutputWsBasename("workspace_test");
  }

  void test_output_basename_set_adds_label_at_before_ws_suffix() {
    std::vector<std::string> test_suffices({"_red", "_sqw"});
    m_presenter->setWsSuffixes(test_suffices);
    EXPECT_CALL(*m_view, enableLabelEditor()).Times(1);
    ON_CALL(*m_view, getCurrentLabel()).WillByDefault(Return("label"));
    EXPECT_CALL(*m_view, setOutputNameLabel("workspace_test_label_red_elwin")).Times(1);
    m_presenter->setOutputWsBasename("workspace_test_red", "_elwin");
  }

private:
  std::unique_ptr<NiceMock<MockOutputNameView>> m_view;
  std::unique_ptr<OutputNamePresenter> m_presenter;
  std::unique_ptr<SetUpADSWithWorkspace> m_ads;
  MatrixWorkspace_sptr m_workspace;
};
