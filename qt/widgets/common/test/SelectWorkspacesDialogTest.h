// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidQtWidgets/Common/SelectWorkspacesDialog.h"
#include <cxxtest/TestSuite.h>
#include <memory>

class SelectWorkspacesDialogTest : public CxxTest::TestSuite {

public:
  static SelectWorkspacesDialogTest *createSuite() { return new SelectWorkspacesDialogTest; }
  static void destroySuite(SelectWorkspacesDialogTest *suite) { delete suite; }

  void test_getSelectedNames_returns_empty_list_if_no_workspace_selected() {
    MantidQt::MantidWidgets::SelectWorkspacesDialog dialog;
    auto selectedNames = dialog.getSelectedNames();
    TS_ASSERT_EQUALS(selectedNames.size(), 0);
  }

  void test_getSelectedNames_returns_selected_workspace_names() {
    MantidQt::MantidWidgets::SelectWorkspacesDialog dialog;
    auto wsList = dialog.findChild<QListWidget *>();
    wsList->addItem("ws1");
    wsList->addItem("ws2");
    wsList->addItem("ws3");
    wsList->item(0)->setSelected(true);
    wsList->item(2)->setSelected(true);
    auto selectedNames = dialog.getSelectedNames();
    TS_ASSERT_EQUALS(selectedNames.size(), 2);
    TS_ASSERT_EQUALS(selectedNames[0], "ws1");
    TS_ASSERT_EQUALS(selectedNames[1], "ws3");
  }

  void test_wsList_is_not_nullptr() {
    MantidQt::MantidWidgets::SelectWorkspacesDialog dialog;
    auto wsList = dialog.findChild<QListWidget *>();
    TS_ASSERT_DIFFERS(wsList, nullptr);
  }

  void test_wsList_has_multi_selection_mode() {
    MantidQt::MantidWidgets::SelectWorkspacesDialog dialog;
    auto wsList = dialog.findChild<QListWidget *>();
    TS_ASSERT_EQUALS(wsList->selectionMode(), QAbstractItemView::MultiSelection);
  }

  void test_wsList_has_extended_selection_mode() {
    MantidQt::MantidWidgets::SelectWorkspacesDialog dialog(nullptr, "", "", QAbstractItemView::ExtendedSelection);
    auto wsList = dialog.findChild<QListWidget *>();
    TS_ASSERT_EQUALS(wsList->selectionMode(), QAbstractItemView::ExtendedSelection);
  }

private:
  std::unique_ptr<MantidQt::MantidWidgets::SelectWorkspacesDialog> m_selectWorkspacesDialog;
};
