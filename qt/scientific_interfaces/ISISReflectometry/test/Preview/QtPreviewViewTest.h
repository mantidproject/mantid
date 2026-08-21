// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "QtPreviewView.h"

#include <QComboBox>
#include <QLabel>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class QtPreviewViewTest : public CxxTest::TestSuite {
public:
  void test_group_member_selector_is_populated_and_shown_for_group() {
    QtPreviewView view;
    auto *selector = view.findChild<QComboBox *>("group_member_combo_box");
    auto *label = view.findChild<QLabel *>("group_member_label");

    view.setGroupMembers({"member_1", "member_2"});

    TS_ASSERT(!selector->isHidden());
    TS_ASSERT(!label->isHidden());
    TS_ASSERT_EQUALS(selector->count(), 2);
    TS_ASSERT_EQUALS(selector->itemText(0).toStdString(), "member_1");
    TS_ASSERT_EQUALS(selector->itemText(1).toStdString(), "member_2");
  }

  void test_group_member_selector_is_cleared_and_hidden_for_matrix_workspace() {
    QtPreviewView view;
    auto *selector = view.findChild<QComboBox *>("group_member_combo_box");
    auto *label = view.findChild<QLabel *>("group_member_label");
    view.setGroupMembers({"member_1", "member_2"});

    view.setGroupMembers({});

    TS_ASSERT(selector->isHidden());
    TS_ASSERT(label->isHidden());
    TS_ASSERT_EQUALS(selector->count(), 0);
  }

  void test_changing_group_member_notifies_subscriber_and_updates_selection() {
    QtPreviewView view;
    Subscriber subscriber;
    view.subscribe(&subscriber);
    view.setGroupMembers({"member_1", "member_2"});
    auto *selector = view.findChild<QComboBox *>("group_member_combo_box");

    selector->setCurrentIndex(1);

    TS_ASSERT_EQUALS(subscriber.selectionChangedCount, 1);
    TS_ASSERT_EQUALS(view.getSelectedGroupMember(), 1);
  }

private:
  class Subscriber final : public PreviewViewSubscriber {
  public:
    void acceptMainPresenter(IBatchPresenter *) override {}
    void notifyLoadWorkspaceRequested() override {}
    void notifyGroupMemberSelectionChanged() override { ++selectionChangedCount; }
    void notifyUpdateAngle() override {}
    void notifyApplyRequested() override {}

    size_t selectionChangedCount{0};
  };
};
