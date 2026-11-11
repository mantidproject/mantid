// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/WorkspaceProperty.h"
#include "MantidDataObjects/WorkspaceSingleValue.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/OptionsPropertyWidget.h"

#include <cxxtest/TestSuite.h>

using namespace Mantid::API;
using namespace Mantid::Kernel;
using MantidQt::API::OptionsPropertyWidget;

class OptionsPropertyWidgetTest : public CxxTest::TestSuite {
public:
  static OptionsPropertyWidgetTest *createSuite() { return new OptionsPropertyWidgetTest(); }
  static void destroySuite(OptionsPropertyWidgetTest *suite) { delete suite; }

  void setUp() override {
    m_showHidden = ConfigService::Instance().getString("MantidOptions.InvisibleWorkspaces");
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", "0");
    auto &ads = AnalysisDataService::Instance();
    ads.addOrReplace("visible", std::make_shared<Mantid::DataObjects::WorkspaceSingleValue>(1.0));
    ads.addOrReplace("__hidden", std::make_shared<Mantid::DataObjects::WorkspaceSingleValue>(1.0));
  }

  void tearDown() override {
    AnalysisDataService::Instance().clear();
    ConfigService::Instance().setString("MantidOptions.InvisibleWorkspaces", m_showHidden);
  }

  void test_hidden_workspace_value_is_selected_rather_than_first_listed() {
    WorkspaceProperty<Workspace> prop("InputWorkspace", "", Direction::Input);
    OptionsPropertyWidget widget(&prop);
    TS_ASSERT_EQUALS(widget.getValue(), "visible");

    widget.setValue("__hidden");

    TS_ASSERT_EQUALS(widget.getValue(), "__hidden");
  }

  void test_value_not_in_ads_keeps_listed_selection() {
    WorkspaceProperty<Workspace> prop("InputWorkspace", "", Direction::Input);
    OptionsPropertyWidget widget(&prop);

    widget.setValue("deleted");

    TS_ASSERT_EQUALS(widget.getValue(), "visible");
  }

private:
  std::string m_showHidden;
};
