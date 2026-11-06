// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "QtPreviewDockedWidgets.h"

#include <QCheckBox>
#include <cxxtest/TestSuite.h>

using namespace MantidQt::CustomInterfaces::ISISReflectometry;

class QtPreviewDockedWidgetsTest : public CxxTest::TestSuite {
public:
  void test_plot_all_group_members_checkbox_is_shown_and_notifies_subscriber() {
    QtPreviewDockedWidgets view;
    Subscriber subscriber;
    view.subscribe(&subscriber);
    auto *checkbox = view.findChild<QCheckBox *>("plot_all_group_members_checkbox");

    view.setPlotAllGroupMembersCheckboxVisible(true);
    checkbox->setChecked(true);

    TS_ASSERT(!checkbox->isHidden());
    TS_ASSERT_EQUALS(checkbox->text().toStdString(), "Plot reduction preview for all group members");
    TS_ASSERT(view.getPlotAllGroupMembers());
    TS_ASSERT_EQUALS(subscriber.plotAllGroupMembersChangedCount, 1);
  }

  void test_hiding_plot_all_group_members_checkbox_resets_it_without_notifying_subscriber() {
    QtPreviewDockedWidgets view;
    Subscriber subscriber;
    view.subscribe(&subscriber);
    auto *checkbox = view.findChild<QCheckBox *>("plot_all_group_members_checkbox");
    view.setPlotAllGroupMembersCheckboxVisible(true);
    checkbox->setChecked(true);

    view.setPlotAllGroupMembersCheckboxVisible(false);

    TS_ASSERT(checkbox->isHidden());
    TS_ASSERT(!view.getPlotAllGroupMembers());
    TS_ASSERT_EQUALS(subscriber.plotAllGroupMembersChangedCount, 1);
  }

private:
  class Subscriber final : public PreviewDockedWidgetsSubscriber {
  public:
    void acceptMainPresenter(IBatchPresenter *) override {}
    void notifyInstViewZoomRequested() override {}
    void notifyInstViewEditRequested() override {}
    void notifyInstViewSelectRectRequested() override {}
    void notifyInstViewShapeChanged() override {}
    void notifyRegionSelectorExportAdsRequested() override {}
    void notifyLinePlotExportAdsRequested() override {}
    void notifyEditROIModeRequested() override {}
    void notifyRectangularROIModeRequested() override {}
    void notifySetYAxisSymlogChanged() override {}
    void notifyPlotAllGroupMembersChanged() override { ++plotAllGroupMembersChangedCount; }

    size_t plotAllGroupMembersChangedCount{0};
  };
};
