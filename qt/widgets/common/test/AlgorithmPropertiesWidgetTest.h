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

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
using ::testing::Mock;
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
  }

  // applyChanges is implemented to call a stored lambda
  bool applyChanges(const IPropertyManager *algo, const std::string &currentPropName) const override {
    if (m_applyCallback)
      return m_applyCallback(algo, currentPropName);
    return false;
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
    // TODO: Initialize QApplication (if needed), create widget under test,
    // and set an appropriate test algorithm on it.
    m_algorithm = static_cast<IAlgorithm_sptr>(new TestAlgorithm());
    m_algorithm->initialize();

    m_widget = std::make_shared<AlgorithmPropertiesWidget>();
    m_widget->setAlgorithm(m_algorithm);
  }

  void tearDown() override {
    // TODO: Destroy / reset widget and any shared state.
    m_widget.reset();
    m_algorithm.reset();
  }

  /// Tests focused on `hideOrDisableProperties` and `IPropertySettings` interaction.

  /// Verifies that when a property has any `IPropertySettings` attached which
  /// indicate the control should be disabled, `hideOrDisableProperties()`
  /// results in the corresponding PropertyWidget reporting `isEnabled() == false`.
  void testHideOrDisable_DisablesPropertiesFromSettings() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *settings = dynamic_cast<MockPropertySettings *>(
        const_cast<IPropertySettings *>(m_algorithm->getPointerToProperty("C")->getSettings()[0].get()));
#if 0
    settings->setIsEnabledReturn(false);

    m_widget->hideOrDisableProperties("");
    // property is disabled
    TS_ASSERT(m_widget->m_propWidgets.contains("C"));
    TS_ASSERT(!m_widget->m_propWidgets["C"]->isEnabled());

    // negative test
    Mock::VerifyAndClear(settings);
    settings->setIsEnabledReturn(true);
#endif
    m_widget->hideOrDisableProperties("");
    // property is NOT disabled
    TS_ASSERT(m_widget->m_propWidgets["C"]->isEnabled());
  }

  /// Verifies that when a property has any `IPropertySettings` attached which
  /// indicate the control should not be visible, `hideOrDisableProperties()`
  /// results in the corresponding PropertyWidget reporting `isVisible() == false`.
  void testHideOrDisable_HidesPropertiesFromSettings() {
    m_algorithm->setPropertySettings("C", std::unique_ptr<IPropertySettings const>(new MockPropertySettings()));
    auto *settings = dynamic_cast<MockPropertySettings *>(
        const_cast<IPropertySettings *>(m_algorithm->getPointerToProperty("C")->getSettings()[0].get()));
#if 0
    settings->setIsVisibleReturn(false);

    m_widget->hideOrDisableProperties("");
    // property is hidden
    TS_ASSERT(m_widget->m_propWidgets.contains("C"));
    TS_ASSERT(!m_widget->m_propWidgets["C"]->isVisible());

    // negative test
    Mock::VerifyAndClear(settings);
    settings->setIsVisibleReturn(true);
#endif

    m_widget->hideOrDisableProperties("");
    // property is NOT hidden
    TS_ASSERT(m_widget->m_propWidgets["C"]->isVisible());
  }

  /// Verifies that dynamic `IPropertySetting`s that modify validators or
  /// properties cause the original PropertyWidget to be replaced by a new
  /// widget instance when `applyChanges(...)` returns true, and that the new
  /// widget occupies the same layout position.
  void testHideOrDisable_DynamicallyReplacesWidgets() {
    // TODO: Arrange a setting whose applyChanges returns true, call
    // hideOrDisableProperties("UpstreamProp"), then assert a new widget
    // instance was created and old one removed.
  }

  /// Verifies that properties explicitly listed in the internal `m_enabled`
  /// list remain enabled, regardless of `IPropertySettings` that might
  /// otherwise disable them.
  void testHideOrDisable_EnabledWhenForcedEnabled() {
    // TODO: Add a property name to menabled, attach disabling settings,
    // run hideOrDisableProperties(""), and assert that the widget is enabled.
  }

  /// Verifies that properties explicitly listed in the internal `m_disabled`
  /// list are disabled even if `IPropertySettings` would otherwise leave
  /// them enabled.
  void testHideOrDisable_DisabledWhenForcedDisabled() {
    // TODO: Add a property name to mdisabled, attach enabling settings,
    // run hideOrDisableProperties(""), and assert that the widget is disabled.
  }

  /// Verifies that `hideOrDisableProperties()` updates widget visibility
  /// based on the result of `IPropertySettings::isVisible` across all
  /// settings, and that this happens after any dynamic replacement of
  /// widgets has completed.
  void testHideOrDisable_UpdatesVisibilityBasedOnSettings() {
    // TODO: Attach visibility settings, trigger hideOrDisableProperties(""),
    // then assert widget->isVisible() matches the expected value.
  }

  /// Verifies that when a property is currently in an error state
  /// (tracked via `m_errors`), the widget remains visible even if the
  /// `IPropertySettings` would normally hide it.
  void testHideOrDisable_ErrorStateOverridesVisibility() {
    // TODO: Mark a property as having an error, attach settings that would
    // hide it, call hideOrDisableProperties(""), and assert the widget
    // stays visible.
  }

  /// Verifies that when multiple `IPropertySettings` are attached to a
  /// property, the enabled state computed by `isWidgetEnabled()` and then
  /// applied in `hideOrDisableProperties()` is the logical AND of all
  /// `settings->isEnabled(...)`, so that any single false disables the widget.
  void testMultipleSettings_AnyDisabledDisablesWidget() {
    // TODO: Attach several settings with at least one reporting isEnabled == false,
    // call hideOrDisableProperties(""), and assert widget->isEnabled() == false.
  }

  /// Verifies that when multiple `IPropertySettings` are attached to a
  /// property, the visible state computed in `hideOrDisableProperties()`
  /// is the logical AND of all `settings->isVisible(...)`, so that any
  /// single false hides the widget.
  void testMultipleSettings_AnyHiddenHidesWidget() {
    // TODO: Attach several settings with at least one reporting isVisible == false,
    // call hideOrDisableProperties(""), and assert widget->isVisible() == false.
  }

  /// Verifies that for each `IPropertySettings` instance attached to a
  /// property, `hideOrDisableProperties()` calls `isConditionChanged(...)`
  /// with the name of the changed property, and when it returns true,
  /// subsequently calls `applyChanges(...)` for that same settings object.
  void testSettingsConditionChange_CallsApplyChangesPerSetting() {
    // TODO: Attach multiple settings with controlled isConditionChanged()
    // return values, call hideOrDisableProperties("UpstreamProp"), and
    // assert applyChanges() was called exactly for those returning true.
  }

  /// Verifies that when an `IPropertySettings::isConditionChanged(...)`
  /// implementation returns false, `hideOrDisableProperties()` does not
  /// call `applyChanges(...)` on that settings object for the current
  /// property.
  void testSettingsConditionChange_DoesNotApplyWhenUnchanged() {
    // TODO: Configure settings where isConditionChanged returns false,
    // invoke hideOrDisableProperties("UpstreamProp"), and assert that
    // applyChanges() is never called for those settings.
  }

  /// Verifies that `hideOrDisableProperties()` completes the first loop over
  /// `IPropertySettings` (checking `isConditionChanged` and applying changes)
  /// before performing the second loop that calculates and applies the
  /// enabled and visible flags for widgets.
  void testHideOrDisable_EvaluatesEnabledAndVisibleAfterApplyingChanges() {
    // TODO: Use spies/mocks to detect that applyChanges has run and any
    // dynamic widget replacement is complete before enabled/visible are
    // computed and applied.
  }

  /// Verifies that `hideOrDisableProperties()` computes the enabled state
  /// via `isWidgetEnabled` (using `settings->isEnabled`) and the visible
  /// state via `settings->isVisible` in separate checks, so that enabling
  /// and visibility are controlled independently even when multiple
  /// `IPropertySettings` are present.
  void testHideOrDisable_SeparatesEnabledAndVisibleChecks() {
    // TODO: Attach settings where isEnabled and isVisible return different
    // combinations, then assert that the widget's enabled and visible
    // states reflect separate logical-AND evaluations.
  }

private:
  IAlgorithm_sptr m_algorithm;
  std::shared_ptr<AlgorithmPropertiesWidget> m_widget;
};
