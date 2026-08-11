// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "GUI/Options/IOptionsDialogModel.h"
#include "MantidKernel/WarningSuppressions.h"
#include <gmock/gmock.h>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {
class MockOptionsDialogModel : public IOptionsDialogModel {
public:
  GNU_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_CONST_METHOD0(defaultSettingsProxy, void());
  OptionsDialogSettings defaultSettings() const override {
    defaultSettingsProxy();
    return OptionsDialogSettings(
        {{"WarnProcessAll", false}, {"WarnDiscardChanges", false}, {"WarnProcessPartialGroup", false}, {"Round", true}},
        {{"RoundPrecision", 5}});
  }
  MOCK_CONST_METHOD0(readSettingsProxy, void());
  OptionsDialogSettings readSettings() const override {
    readSettingsProxy();
    return OptionsDialogSettings(
        {{"WarnProcessAll", false}, {"WarnDiscardChanges", true}, {"WarnProcessPartialGroup", false}, {"Round", true}},
        {{"RoundPrecision", 2}});
  }
  MOCK_METHOD1(saveSettingsProxy, void(OptionsDialogSettings const &));
  void saveSettings(OptionsDialogSettings const &settings) override { saveSettingsProxy(settings); }
};

class MockOptionsDialogModelUnsuccessfulLoad : public IOptionsDialogModel {
public:
  MOCK_CONST_METHOD0(defaultSettings, OptionsDialogSettings());
  MOCK_CONST_METHOD0(readSettings, OptionsDialogSettings());
  MOCK_METHOD1(saveSettings, void(OptionsDialogSettings const &));
};

class MockOptionsDialogModelUnsuccessfulDefaults : public IOptionsDialogModel {
public:
  MOCK_CONST_METHOD0(defaultSettingsProxy, void());
  OptionsDialogSettings defaultSettings() const override {
    defaultSettingsProxy();
    return OptionsDialogSettings(
        {{"WarnProcessAll", false}, {"WarnDiscardChanges", false}, {"WarnProcessPartialGroup", false}, {"Round", true}},
        {{"RoundPrecision", 5}});
  }
  MOCK_CONST_METHOD0(readSettings, OptionsDialogSettings());
  MOCK_METHOD1(saveSettings, void(OptionsDialogSettings const &));
};
GNU_DIAG_ON_SUGGEST_OVERRIDE
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
