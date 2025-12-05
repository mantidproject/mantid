// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/IPropertyManager.h"
#include "MantidKernel/IPropertySettings.h"
#include "MantidQtWidgets/Common/AlgorithmPropertiesWidget.h"

using namespace Mantid::API;
using namespace Mantid::Kernel;

// #include <QApplication>
#include <QString>
#include <QTimer>

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

class AlgorithmPropertiesWidgetTest : public CxxTest::TestSuite {
public:
  static AlgorithmPropertiesWidgetTest *createSuite() { return new AlgorithmPropertiesWidgetTest(); }
  static void destroySuite(AlgorithmPropertiesWidgetTest *suite) { delete suite; }

  void setUp() override;
  void tearDown() override;

  // High‑level behaviour tests
  void testHideOrDisable_DisablesPropertiesWithSettings() {}
  void testHideOrDisable_DynamicallyReplacesWidgets() {}
  void testHideOrDisable_EnabledWhenForcedEnabled() {}
  void testHideOrDisable_DisabledWhenForcedDisabled() {}
  void testHideOrDisable_UpdatesVisibilityBasedOnValidators() {}
  void testHideOrDisable_ErrorStateOverridesVisibility() {}

  // New tests for multiple IPropertySettings and ordering

  // (1) Enabled flag = logical AND of all settings->isEnabled(...)
  void testMultipleSettings_AnyDisabledDisablesWidget() {}

  // (2) Visible flag = logical AND of all settings->isVisible(...)
  void testMultipleSettings_AnyHiddenHidesWidget() {}

  // (3a) isConditionChanged/applyChanges called for each setting, in first loop
  void testSettingsConditionChange_CallsApplyChangesPerSetting() {}

  // (3b) isConditionChangedFalse_DoesNotCallApplyChanges() {}
  void testSettingsConditionChange_DoesNotApplyWhenUnchanged() {}

  // (3c) Enabled/visible evaluation occurs after condition/applyChanges loop
  void testHideOrDisable_EvaluatesEnabledAndVisibleAfterApplyChanges() {}

  // (3d) Separate evaluation of enabled vs visible, both using logical AND
  void testHideOrDisable_SeparatesEnabledAndVisibleChecks() {}
};
