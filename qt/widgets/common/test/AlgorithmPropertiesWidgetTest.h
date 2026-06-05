// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Algorithm.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidKernel/PropertyWithValue.h"
#include "MantidQtWidgets/Common/AlgorithmPropertiesWidget.h"
#include <QCoreApplication>
#include <QEventLoop>
#include <QMetaObject>
#include <QWidget>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
using ::testing::_;
using ::testing::InSequence;
using ::testing::Mock;
using ::testing::Sequence;
#include <gtest/gtest.h>

// Tests for AlgorithmPropertiesWidget.

// TODO: These tests are presently focused on hideOrDisableProperties
// and IPropertySettings interaction.  More complete behavior tests should also
// be implemented as time permits.

namespace Mantid {
namespace API {

class TestAlgorithm final : public Algorithm {
public:
  /// Factory-style shared pointer typedef if needed
  using sptr = std::shared_ptr<TestAlgorithm>;

  TestAlgorithm() = default;
  ~TestAlgorithm() override = default;

  /// Algorithm information
  const std::string name() const override { return "TestAlgorithm"; }
  int version() const override { return 1; }
  const std::string summary() const override { return "Test algorithm with three float input properties A, B, and C."; }

protected:
  /// Declare properties
  void init() override {
    using Mantid::Kernel::Direction;
    using Mantid::Kernel::PropertyWithValue;

    declareProperty(std::make_unique<PropertyWithValue<double>>("A", 0.0, Direction::Input), "Input value A");

    declareProperty(std::make_unique<PropertyWithValue<double>>("B", 0.0, Direction::Input), "Input value B");

    declareProperty(std::make_unique<PropertyWithValue<double>>("C", 0.0, Direction::Input), "Input value C");
  }

  /// Execution stub
  void exec() override {
    // Stub implementation for unit tests; do nothing or minimal access
    // auto a = getProperty("A");
    // auto b = getProperty("B");
    // auto c = getProperty("C");
  }
};

} // namespace API
} // namespace Mantid

namespace Mantid {
namespace Kernel {

class MockPropertySettings : public IPropertySettings {
public:
  using ApplyCallback = std::function<bool(const IPropertyManager *, const std::string &)>;

  MOCK_METHOD(bool, isEnabled, (const IPropertyManager *), (const, override));
  MOCK_METHOD(bool, isVisible, (const IPropertyManager *), (const, override));
  MOCK_METHOD(bool, isConditionChanged, (const IPropertyManager *, const std::string &), (const, override));
  MOCK_METHOD(bool, applyChanges, (const IPropertyManager *algo, const std::string &currentPropName),
              (const, override));

  MockPropertySettings() {
    ON_CALL(*this, isEnabled(testing::_)).WillByDefault(testing::Invoke([this](const IPropertyManager *) {
      return m_isEnabledDefault;
    }));
    ON_CALL(*this, isVisible(testing::_)).WillByDefault(testing::Invoke([this](const IPropertyManager *) {
      return m_isVisibleDefault;
    }));
    ON_CALL(*this, isConditionChanged(testing::_, testing::_))
        .WillByDefault(testing::Invoke(
            [this](const IPropertyManager *, const std::string &) { return m_isConditionChangedDefault; }));
    ON_CALL(*this, applyChanges(testing::_, testing::_))
        .WillByDefault(testing::Invoke([this](const IPropertyManager *algo, const std::string &name) {
          if (m_applyCallback)
            return m_applyCallback(algo, name);
          return false;
        }));
  }

  IPropertySettings *clone() const override {
    auto *copy = new ::testing::NiceMock<MockPropertySettings>();

    // Copy configuration state
    copy->m_isEnabledDefault = m_isEnabledDefault;
    copy->m_isVisibleDefault = m_isVisibleDefault;
    copy->m_isConditionChangedDefault = m_isConditionChangedDefault;
    copy->m_applyCallback = m_applyCallback;

    // Constructor of copy has already installed ON_CALLs that use its members.
    return copy;
  }

  void setIsEnabledReturn(bool v) { m_isEnabledDefault = v; }
  void setIsVisibleReturn(bool v) { m_isVisibleDefault = v; }
  void setIsConditionChangedReturn(bool v) { m_isConditionChangedDefault = v; }
  void setApplyChangesCallback(ApplyCallback cb) { m_applyCallback = std::move(cb); }

private:
  bool m_isEnabledDefault{true};
  bool m_isVisibleDefault{true};
  bool m_isConditionChangedDefault{false};
  ApplyCallback m_applyCallback;
};

} // namespace Kernel
} // namespace Mantid

using namespace Mantid::API;
using namespace Mantid::Kernel;
using namespace MantidQt::API;

class AlgorithmPropertiesWidgetTest : public CxxTest::TestSuite {
public:
  static AlgorithmPropertiesWidgetTest *createSuite() { return new AlgorithmPropertiesWidgetTest(); }
  static void destroySuite(AlgorithmPropertiesWidgetTest *suite) { delete suite; }

  void setUp() override {
    // QApplication is initialized at `WidgetsCommonTestInitialization.h`
    //   which should be automatically included by the test framework.

    // Ensure widget is part of a shown hierarchy
    m_parent = new QWidget();
    m_parent->show();

    m_algorithm = static_cast<IAlgorithm_sptr>(new TestAlgorithm());
    m_algorithm->initialize();

    m_widget = std::make_shared<TestableAlgorithmPropertiesWidget>(m_parent);
    m_widget->setAlgorithm(m_algorithm);

    QCoreApplication::processEvents();
  }

  void tearDown() override {
    // Reset widget and any shared state.
    m_widget.reset();
    m_algorithm.reset();

    delete m_parent;
    m_parent = nullptr;
  }

  /// Tests focused on `hideOrDisableProperties` and `IPropertySettings` interaction.

  /// Verifies that when a property has any `IPropertySettings` attached which
  /// indicate the control should be disabled, `isWidgetEnabled()`
  /// returns the correct value.
  void testIsWidgetEnabled_DisablesPropertiesFromSettings() {
    // WARNING: in a headless tests, it is problematic to test this directly using `QWidget::isEnabled()`.
    // The private helper method `isWidgetEnabled` is verified instead.

    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));

    settings->setIsEnabledReturn(false);
    TS_ASSERT(!m_widget->isWidgetEnabled(prop));

    settings->setIsEnabledReturn(true);
    TS_ASSERT(m_widget->isWidgetEnabled(prop));
  }

  /// Verifies that when a property has any `IPropertySettings` attached which
  /// indicate the control should be hidden, `isWidgetVisible()`
  /// returns the correct value.
  void testIsWidgetVisible_HidesPropertiesFromSettings() {
    // WARNING: in a headless tests, it is problematic to test visibility directly using `QWidget::isVisible()`.
    // For this reason, the private helper method `isWidgetVisible` is verified instead.

    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));

    settings->setIsVisibleReturn(false);
    TS_ASSERT(!m_widget->isWidgetVisible(prop));

    settings->setIsVisibleReturn(true);
    TS_ASSERT(m_widget->isWidgetVisible(prop));
  }

  /// Verifies that when a property's validators indicate an error condition,
  /// `isWidgetVisible()` always returns `true`, regardless of property-settings' state.
  void testIsWidgetVisible_DoesNotHideErrors() {
    // Note: this behavior did not work previously. `AlgorithmDialog` and `AlgorithmPropertiesWidget` retained
    //   separate `m_errors` maps, and the latter no longer actually set any properties values!

    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));

    settings->setIsVisibleReturn(false);
    TS_ASSERT(!m_widget->isWidgetVisible(prop));

    QHash<QString, QString> errors = {{"C", "something is not right!"}};
    m_widget->shareErrorsMap(errors);
    TS_ASSERT(m_widget->isWidgetVisible(prop));
  }

  /// Verifies that dynamic `IPropertySetting`s that modify validators or
  /// properties cause the original PropertyWidget to be replaced by a new
  /// widget instance when `applyChanges` returns true, and that the new
  /// widget occupies the same layout position.
  void testHideOrDisable_DynamicallyReplacesWidgets() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));

    // First test the negative: widget remains the same when `applyChanges` returns `false`.
    settings->setIsConditionChangedReturn(true);
    MockPropertySettings::ApplyCallback cb0 = [](const IPropertyManager *, const std::string &) { return false; };
    settings->setApplyChangesCallback(cb0);
    const auto *originalWidget = m_widget->m_propWidgets["C"];
    int originalRow = originalWidget->getGridRow();

    m_widget->hideOrDisableProperties("A"); // in this test, the name of the upstream property doesn't matter
    TS_ASSERT(m_widget->m_propWidgets["C"] == originalWidget);
    TS_ASSERT(m_widget->m_propWidgets["C"]->getGridRow() == originalRow);

    // Next verify that the `PropertyWidget` is replaced when the `IPropertySettings::applyChanges` returns `true`.
    MockPropertySettings::ApplyCallback cb1 = [](const IPropertyManager *, const std::string &) { return true; };
    settings->setApplyChangesCallback(cb1);
    m_widget->hideOrDisableProperties("A"); // in this test, the name of the upstream property doesn't matter
    TS_ASSERT(m_widget->m_propWidgets["C"] != originalWidget);
    TS_ASSERT(m_widget->m_propWidgets["C"]->getGridRow() == originalRow);
  }

  /// Verifies that properties explicitly listed in the internal `m_enabled`
  /// list remain enabled, regardless of `IPropertySettings` that might
  /// otherwise disable them.
  void testIsWidgetEnabled_EnabledWhenForcedEnabled() {
    // For reasons discussed previously RE GUI testing:
    //   we test `isWidgetEnabled(prop)` rather than `<property>.isEnabled()`.

    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));
    settings->setIsEnabledReturn(false);

    {
      QStringList enabled = {"C"}, disabled = {};
      m_widget->addEnabledAndDisableLists(enabled, disabled);
      TS_ASSERT(m_widget->isWidgetEnabled(prop));
    }

    {
      // negative case
      QStringList enabled = {}, disabled = {};
      m_widget->addEnabledAndDisableLists(enabled, disabled);
      TS_ASSERT(!m_widget->isWidgetEnabled(prop));
    }
  }

  /// Verifies that properties explicitly listed in the internal `m_disabled`
  /// list are disabled even if `IPropertySettings` would otherwise leave
  /// them enabled.
  void testIsWidgetEnabled_DisabledWhenForcedDisabled() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));
    settings->setIsEnabledReturn(true);

    {
      QStringList enabled = {}, disabled = {"C"};
      m_widget->addEnabledAndDisableLists(enabled, disabled);
      TS_ASSERT(!m_widget->isWidgetEnabled(prop));
    }

    {
      // negative case
      QStringList enabled = {}, disabled = {};
      m_widget->addEnabledAndDisableLists(enabled, disabled);
      TS_ASSERT(m_widget->isWidgetEnabled(prop));
    }
  }

  /// Verifies that properties explicitly listed in the internal `m_enabled`
  /// list are enabled even if they are also listed in the `m_disabled` list.
  void testIsWidgetEnabled_ForcedEnabledSupercedesForcedDisabled() {
    auto *prop = m_algorithm->getPointerToProperty("C");
    {
      QStringList enabled = {"C"}, disabled = {"C"};
      m_widget->addEnabledAndDisableLists(enabled, disabled);
      TS_ASSERT(m_widget->isWidgetEnabled(prop));
    }

    {
      // negative case
      QStringList enabled = {"C"}, disabled = {};
      m_widget->addEnabledAndDisableLists(enabled, disabled);
      TS_ASSERT(m_widget->isWidgetEnabled(prop));
    }
  }

  /// Verifies that when multiple `IPropertySettings` are attached to a
  /// property, the enabled state computed by `isWidgetEnabled()` and then
  /// applied in `hideOrDisableProperties()` is the logical AND of all
  /// `settings->isEnabled(...)`, so that any single false disables the widget.
  void testMultipleSettings_AnyDisabledDisablesWidget() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings1 =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));
    auto *settings2 =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[1].get()));

    // verify the AND truth table
    settings1->setIsEnabledReturn(false);
    settings2->setIsEnabledReturn(false);
    TS_ASSERT_EQUALS(m_widget->isWidgetEnabled(prop), false);

    settings1->setIsEnabledReturn(false);
    settings2->setIsEnabledReturn(true);
    TS_ASSERT_EQUALS(m_widget->isWidgetEnabled(prop), false);

    settings1->setIsEnabledReturn(true);
    settings2->setIsEnabledReturn(false);
    TS_ASSERT_EQUALS(m_widget->isWidgetEnabled(prop), false);

    settings1->setIsEnabledReturn(true);
    settings2->setIsEnabledReturn(true);
    TS_ASSERT_EQUALS(m_widget->isWidgetEnabled(prop), true);
  }

  /// Verifies that when no `IPropertySettings` are attached to a
  /// property, the enabled state computed by `isWidgetEnabled()` is `true`.
  void testSettings_WidgetEnabledByDefault() {
    auto *prop = m_algorithm->getPointerToProperty("C");
    TS_ASSERT(prop->getSettings().empty());
    TS_ASSERT_EQUALS(m_widget->isWidgetEnabled(prop), true);
  }

  /// Verifies that when multiple `IPropertySettings` are attached to a
  /// property, the visible state computed by `isWidgetVisible` and then
  /// applied in `hideOrDisableProperties()` is the logical AND of all
  /// `settings->isVisible(...)`, so that any single false hides the widget.
  void testMultipleSettings_AnyHiddenHidesWidget() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings1 =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));
    auto *settings2 =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[1].get()));

    // verify the AND truth table
    settings1->setIsVisibleReturn(false);
    settings2->setIsVisibleReturn(false);
    TS_ASSERT_EQUALS(m_widget->isWidgetVisible(prop), false);

    settings1->setIsVisibleReturn(false);
    settings2->setIsVisibleReturn(true);
    TS_ASSERT_EQUALS(m_widget->isWidgetVisible(prop), false);

    settings1->setIsVisibleReturn(true);
    settings2->setIsVisibleReturn(false);
    TS_ASSERT_EQUALS(m_widget->isWidgetVisible(prop), false);

    settings1->setIsVisibleReturn(true);
    settings2->setIsVisibleReturn(true);
    TS_ASSERT_EQUALS(m_widget->isWidgetVisible(prop), true);
  }

  /// Verifies that when no `IPropertySettings` are attached to a
  /// property, the visibility state computed by `isWidgetVisible()` is `true`.
  void testSettings_WidgetVisibleByDefault() {
    auto *prop = m_algorithm->getPointerToProperty("C");
    TS_ASSERT(prop->getSettings().empty());
    TS_ASSERT_EQUALS(m_widget->isWidgetVisible(prop), true);
  }

  /// Verifies that for each `IPropertySettings` instance attached to a
  /// property, `hideOrDisableProperties()` calls `isConditionChanged(...)`
  /// with the name of the changed property, and when it returns true,
  /// subsequently calls `applyChanges(...)` for that same settings object.
  void testSettingsConditionChange_DoesNotApplyWhenUnchanged() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop = m_algorithm->getPointerToProperty("C");
    auto *settings1 =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[0].get()));
    auto *settings2 =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop->getSettings()[1].get()));

    // Both positive and negative cases are checked.
    settings1->setIsConditionChangedReturn(false);
    settings2->setIsConditionChangedReturn(true);
    EXPECT_CALL(*settings1, isConditionChanged(m_algorithm.get(), "A")).Times(1);
    EXPECT_CALL(*settings1, applyChanges(_, _)).Times(0);
    EXPECT_CALL(*settings2, isConditionChanged(m_algorithm.get(), "A")).Times(1);
    EXPECT_CALL(*settings2, applyChanges(m_algorithm.get(), "C")).Times(1);

    m_widget->hideOrDisableProperties("A");

    // The following prevents unmet expectations from hanging the framework,
    //   due to `ctest`, `gtest` and `Qt` interactions during shutdown.
    Mock::VerifyAndClearExpectations(settings1);
    Mock::VerifyAndClearExpectations(settings2);
  }

  /// Verifies that the  `IPropertySettings::isConditionChanged` => `IPropertySettings::applyChanges`
  /// are called separately for each property in sequence, and are not executed as a composite.
  void testHideOrDisable_AppliesChangesInSequence() {
    // Implementation note: the primary purpose of this test is to provide a reality check, in case
    // code is ever implemented that does a composite `isConditionChanged` check over all
    // properties (and their settings), and then based on that calls all of the `applyChanges`.
    // This *might* work out OK, but whether or not it leads to correct behavior in all cases
    // needs to be carefully evaluated.

    m_algorithm->setPropertySettings("B", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop_b = m_algorithm->getPointerToProperty("B");
    auto *settings_b =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop_b->getSettings()[0].get()));
    auto *prop_c = m_algorithm->getPointerToProperty("C");
    auto *settings_c =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop_c->getSettings()[0].get()));
    settings_b->setIsConditionChangedReturn(true);
    settings_c->setIsConditionChangedReturn(true);

    // The specific sequence of calls is checked,
    // but we don't care about the ordering of the properties
    // in the widget's list.
    InSequence seq;
    int count = 0;
    for (auto &widget : m_widget->m_propWidgets) {
      Mantid::Kernel::Property *prop = widget->getProperty();
      auto *settings = dynamic_cast<MockPropertySettings *>(
          const_cast<IPropertySettings *>(!prop->getSettings().empty() ? prop->getSettings()[0].get() : nullptr));
      if (settings) {
        EXPECT_CALL(*settings, isConditionChanged(m_algorithm.get(), "A")).Times(1);
        EXPECT_CALL(*settings, applyChanges(m_algorithm.get(), prop->name())).Times(1);
        ++count;
      }
    }
    TS_ASSERT_EQUALS(count, 2);

    m_widget->hideOrDisableProperties("A");

    // The following prevents unmet expectations from hanging the framework,
    //   due to `ctest`, `gtest` and `Qt` interactions during shutdown.
    Mock::VerifyAndClearExpectations(settings_b);
    Mock::VerifyAndClearExpectations(settings_c);
  }

  /// Verifies that `hideOrDisableProperties()` completes the first loop over
  /// `IPropertySettings` (checking `isConditionChanged` and applying changes)
  /// before performing the second loop that calculates and applies the
  /// enabled and visible flags for widgets.
  void testHideOrDisable_EvaluatesEnabledAndVisibleAfterApplyingAllChanges() {
    m_algorithm->setPropertySettings("B", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *prop_b = m_algorithm->getPointerToProperty("B");
    auto *settings_b =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop_b->getSettings()[0].get()));
    auto *prop_c = m_algorithm->getPointerToProperty("C");
    auto *settings_c =
        dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(prop_c->getSettings()[0].get()));
    settings_b->setIsConditionChangedReturn(true);
    settings_c->setIsConditionChangedReturn(true);

    // The specific sequence of calls is checked,
    // but we don't care about the ordering of the properties
    // in the widget's list.
    InSequence seq;
    int count = 0;
    for (auto &widget : m_widget->m_propWidgets) {
      Mantid::Kernel::Property *prop = widget->getProperty();
      auto *settings = dynamic_cast<MockPropertySettings *>(
          const_cast<IPropertySettings *>(!prop->getSettings().empty() ? prop->getSettings()[0].get() : nullptr));
      if (settings) {
        EXPECT_CALL(*settings, isConditionChanged(m_algorithm.get(), "A")).Times(1);
        EXPECT_CALL(*settings, applyChanges(m_algorithm.get(), prop->name())).Times(1);
        ++count;
      }
    }
    for (auto &widget : m_widget->m_propWidgets) {
      Mantid::Kernel::Property *prop = widget->getProperty();
      auto *settings = dynamic_cast<MockPropertySettings *>(
          const_cast<IPropertySettings *>(!prop->getSettings().empty() ? prop->getSettings()[0].get() : nullptr));
      if (settings) {
        EXPECT_CALL(*settings, isEnabled(m_algorithm.get())).Times(1);
        EXPECT_CALL(*settings, isVisible(m_algorithm.get())).Times(1);
        ++count;
      }
    }
    TS_ASSERT_EQUALS(count, 4);

    m_widget->hideOrDisableProperties("A");

    // The following prevents unmet expectations from hanging the framework,
    //   due to `ctest`, `gtest` and `Qt` interactions during shutdown.
    Mock::VerifyAndClearExpectations(settings_b);
    Mock::VerifyAndClearExpectations(settings_c);
  }

  /// Regression test for a use-after-free / pure-virtual crash that occurred when
  /// `initLayout()` was called a second time on a widget that already had
  /// PropertyWidgets and their associated infoWidgets (icon-strip QWidgets living
  /// in column 4 of the grid).
  ///
  /// Root cause: `initLayout()` removed items from the grid layout first (queuing
  /// infoWidget deletions), then queued PropertyWidget deletions, then called
  /// `processEvents()`. Since infoWidgets were processed (freed) before their
  /// PropertyWidgets, the `destroyed`-signal lambda on each PropertyWidget
  /// subsequently called `deleteLater()` on already-freed memory. The stale
  /// DeferredDelete event caused a pure-virtual call via the freed vtable when
  /// next processed, triggering `std::terminate()` via Mantid's terminate handler.
  ///
  /// Fix: PropertyWidgets must be queued for deletion before grid widgets so that
  /// the lambda fires while the infoWidget is still alive.
  ///
  /// See: https://github.com/mantidproject/mantid/issues/41471
  void testInitLayout_ReinitializationDoesNotUseAfterFree() {
    // The bug is a deletion-order violation introduced in PR #40132.
    // initLayout() must queue PropertyWidget deletions BEFORE grid-widget deletions
    // (which include the infoWidgets at column 4). If the order is reversed:
    //
    //   1. infoWidgets get deleteLater() first.
    //   2. PropWidgets get deleteLater() second.
    //   3. processEvents() fires DeferredDelete events FIFO.
    //   4. infoWidget is freed; its memory may be reused.
    //   5. propWidget's `destroyed`-signal lambda calls infoWidget->deleteLater()
    //      on freed / reused memory.
    //   6. The stale DeferredDelete reaches a garbage vtable → pure-virtual crash.
    //
    // Fix: propWidgets queued first → lambda fires while infoWidget is still alive.
    //
    // Test strategy: connect to the `destroyed` signals of both the OLD infoWidgets
    // and OLD propWidgets. Count how many infoWidgets fire `destroyed` before any
    // propWidget does. With the bug that count will be > 0; with the fix it is 0.
    //
    // Important: Qt only processes DeferredDelete events when the event-loop level
    // is exactly (post_level + 1). Because tests run outside any event loop
    // (loopLevel = 0), QCoreApplication::processEvents() alone cannot drain them.
    // Running a brief QEventLoop bumps the level to 1 and lets Qt flush the queue
    // while this stack frame — and therefore the shared_ptr captures — is still alive.

    QGridLayout *inputGrid = m_widget->m_propWidgets.begin().value()->getGridLayout();

    // Count OLD infoWidgets (column 4) that have already fired `destroyed` by the
    // time the first OLD propWidget fires `destroyed`.
    auto infoWidgetsDestroyedCount = std::make_shared<int>(0);
    int totalInfoWidgets = 0;
    for (int row = 0; row < inputGrid->rowCount(); ++row) {
      auto *item = inputGrid->itemAtPosition(row, 4);
      if (item && item->widget()) {
        ++totalInfoWidgets;
        QObject::connect(item->widget(), &QObject::destroyed,
                         [infoWidgetsDestroyedCount]() { (*infoWidgetsDestroyedCount)++; });
      }
    }
    TS_ASSERT_LESS_THAN(0, totalInfoWidgets);

    // Bug present  → infoWidgets freed first → count > 0 when propWidget fires → flag set.
    // Bug fixed    → propWidgets freed first → count == 0 when propWidget fires → flag clear.
    auto infoWidgetDestroyedBeforePropWidget = std::make_shared<bool>(false);
    for (auto *propWidget : m_widget->m_propWidgets) {
      QObject::connect(propWidget, &QObject::destroyed,
                       [infoWidgetsDestroyedCount, infoWidgetDestroyedBeforePropWidget]() {
                         if (*infoWidgetsDestroyedCount > 0)
                           *infoWidgetDestroyedBeforePropWidget = true;
                       });
    }

    // Trigger the cleanup.
    m_widget->setAlgorithm(m_algorithm);

    // Flush DeferredDelete events. QEventLoop::exec() raises the loopLevel to 1,
    // which satisfies Qt's condition for processing events posted at level 0.
    // The invokeMethod queues the quit *after* the deletions so the loop exits
    // only once all DeferredDelete events have been dispatched.
    {
      QEventLoop loop;
      QMetaObject::invokeMethod(&loop, "quit", Qt::QueuedConnection);
      loop.exec();
    }

    // Sanity check: all OLD infoWidgets must have been destroyed.
    // If this fails, the event-loop flush above did not process DeferredDelete events.
    TS_ASSERT_EQUALS(*infoWidgetsDestroyedCount, totalInfoWidgets);

    TS_ASSERT(!*infoWidgetDestroyedBeforePropWidget);
  }

  /// Verifies that `hideOrDisableProperties()` computes the enabled state
  /// via `isWidgetEnabled` (iterating over all `settings->isEnabled()`)
  /// and the visible state via `isWidgetVisible` (iterating over all `settings->isVisible()`)
  /// in separate checks, so that enabling and visibility are controlled independently even when multiple
  /// `IPropertySettings` are present.
  void testHideOrDisable_SeparatesEnabledAndVisibleChecks() {
    m_algorithm->setPropertySettings("B", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("B", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));

    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));

    // The specific sequence of calls is checked,
    // but we don't care about the ordering of the properties
    // in the widget's list.
    InSequence seq;
    int count = 0;
    for (auto &widget : m_widget->m_propWidgets) {
      Mantid::Kernel::Property *prop = widget->getProperty();
      // Verify that each settings chain for `isEnabled` and `isVisible` is iterated separately,
      // and that they are not interspersed.
      // (NOTE: with respect to the code itself,
      //  we probably don't care that `isEnabled` is checked before or after `isVisible`.)
      for (const auto &settings : prop->getSettings()) {
        auto *settings_ = dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(settings.get()));
        EXPECT_CALL(*settings_, isEnabled(m_algorithm.get())).Times(1);
        ++count;
      }
      for (const auto &settings : prop->getSettings()) {
        auto *settings_ = dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(settings.get()));
        EXPECT_CALL(*settings_, isVisible(m_algorithm.get())).Times(1);
        ++count;
      }
    }
    TS_ASSERT_EQUALS(count, 8);

    m_widget->hideOrDisableProperties("A");

    // The following prevents unmet expectations from hanging the framework,
    //   due to `ctest`, `gtest` and `Qt` interactions during shutdown.
    for (auto &widget : m_widget->m_propWidgets) {
      Mantid::Kernel::Property *prop = widget->getProperty();
      for (const auto &settings : prop->getSettings()) {
        auto *settings_ = dynamic_cast<MockPropertySettings *>(const_cast<IPropertySettings *>(settings.get()));
        Mock::VerifyAndClearExpectations(settings_);
      }
    }
  }

private:
  class TestableAlgorithmPropertiesWidget : public AlgorithmPropertiesWidget {
    // This class allows verification of `isWidgetEnabled` and `isWidgetVisible`.
    friend class AlgorithmPropertiesWidgetTest;

  public:
    TestableAlgorithmPropertiesWidget(QWidget *parent = nullptr) : AlgorithmPropertiesWidget(parent) {}
    ~TestableAlgorithmPropertiesWidget() override = default;
  };

  QWidget *m_parent = nullptr;
  std::shared_ptr<TestableAlgorithmPropertiesWidget> m_widget;
  IAlgorithm_sptr m_algorithm;
};
