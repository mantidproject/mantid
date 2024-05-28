// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidKernel/WarningSuppressions.h"

#include "Common/OutputPlotOptionsModel.h"
#include "Common/OutputPlotOptionsView.h"
#include "Common/Settings.h"
#include "Common/SettingsModel.h"
#include "Common/SettingsView.h"

#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockOutputPlotOptionsView : public IOutputPlotOptionsView {
public:
  virtual ~MockOutputPlotOptionsView() = default;

  MOCK_METHOD1(subscribePresenter, void(IOutputPlotOptionsPresenter *));
  MOCK_METHOD2(setPlotType,
               void(PlotWidget const &plotType, std::map<std::string, std::string> const &availableActions));

  MOCK_METHOD1(setIndicesRegex, void(QString const &regex));

  MOCK_CONST_METHOD0(selectedWorkspace, QString());
  MOCK_METHOD1(setWorkspaces, void(std::vector<std::string> const &workspaces));

  MOCK_METHOD1(removeWorkspace, void(QString const &workspaceName));
  MOCK_METHOD0(clearWorkspaces, void());

  MOCK_CONST_METHOD0(selectedIndices, QString());
  MOCK_METHOD1(setIndices, void(QString const &indices));
  MOCK_METHOD1(setIndicesErrorLabelVisible, void(bool visible));

  MOCK_METHOD1(setWorkspaceComboBoxEnabled, void(bool enable));
  MOCK_METHOD1(setUnitComboBoxEnabled, void(bool enable));
  MOCK_METHOD1(setIndicesLineEditEnabled, void(bool enable));
  MOCK_METHOD1(setPlotButtonEnabled, void(bool enable));
  MOCK_METHOD1(setPlotButtonText, void(QString const &text));

  MOCK_CONST_METHOD0(numberOfWorkspaces, int());

  MOCK_METHOD1(addIndicesSuggestion, void(QString const &spectra));

  MOCK_METHOD1(displayWarning, void(QString const &message));
};

class MockOutputPlotOptionsModel : public IOutputPlotOptionsModel {
public:
  virtual ~MockOutputPlotOptionsModel() = default;

  MOCK_METHOD1(setWorkspace, bool(std::string const &workspaceName));
  MOCK_METHOD0(removeWorkspace, void());

  MOCK_CONST_METHOD1(getAllWorkspaceNames, std::vector<std::string>(std::vector<std::string> const &workspaceNames));
  MOCK_CONST_METHOD0(workspace, std::optional<std::string>());

  MOCK_METHOD1(setFixedIndices, void(std::string const &indices));
  MOCK_CONST_METHOD0(indicesFixed, bool());

  MOCK_METHOD1(setUnit, void(std::string const &unit));
  MOCK_METHOD0(unit, std::optional<std::string>());

  MOCK_CONST_METHOD1(formatIndices, std::string(std::string const &indices));
  MOCK_CONST_METHOD2(validateIndices, bool(std::string const &indices, MantidAxis const &axisType));
  MOCK_METHOD1(setIndices, bool(std::string const &indices));
  MOCK_CONST_METHOD0(indices, std::optional<std::string>());

  MOCK_METHOD0(plotSpectra, void());
  MOCK_METHOD1(plotBins, void(std::string const &binIndices));
  MOCK_METHOD0(showSliceViewer, void());
  MOCK_METHOD0(plotTiled, void());
  MOCK_METHOD0(plot3DSurface, void());

  MOCK_CONST_METHOD1(singleDataPoint, std::optional<std::string>(MantidAxis const &axisType));
  MOCK_CONST_METHOD0(availableActions, std::map<std::string, std::string>());
};

class MockSettingsView : public ISettingsView {
public:
  virtual ~MockSettingsView() = default;

  MOCK_METHOD0(getView, QWidget *());
  MOCK_METHOD1(subscribePresenter, void(SettingsPresenter *));

  MOCK_METHOD1(setInterfaceSettingsVisible, void(bool visible));
  MOCK_METHOD1(setInterfaceGroupBoxTitle, void(QString const &title));

  MOCK_METHOD1(setRestrictInputByNameVisible, void(bool visible));
  MOCK_METHOD1(setPlotErrorBarsVisible, void(bool visible));

  MOCK_METHOD1(setSelectedFacility, void(QString const &text));
  MOCK_CONST_METHOD0(getSelectedFacility, QString());

  MOCK_METHOD1(setRestrictInputByNameChecked, void(bool check));
  MOCK_CONST_METHOD0(isRestrictInputByNameChecked, bool());

  MOCK_METHOD1(setPlotErrorBarsChecked, void(bool check));
  MOCK_CONST_METHOD0(isPlotErrorBarsChecked, bool());

  MOCK_METHOD1(setDeveloperFeatureFlags, void(QStringList const &flags));
  MOCK_CONST_METHOD0(developerFeatureFlags, QStringList());

  MOCK_METHOD3(setSetting, void(QString const &settingsGroup, QString const &settingName, bool const &value));
  MOCK_METHOD2(getSetting, QVariant(QString const &settingsGroup, QString const &settingName));

  MOCK_METHOD1(setApplyText, void(QString const &text));
  MOCK_METHOD1(setApplyEnabled, void(bool enable));
  MOCK_METHOD1(setOkEnabled, void(bool enable));
  MOCK_METHOD1(setCancelEnabled, void(bool enable));
};

class MockSettingsModel : public SettingsModel {
public:
  virtual ~MockSettingsModel() = default;

  MOCK_CONST_METHOD0(getSettingsGroup, std::string());

  MOCK_METHOD1(setFacility, void(std::string const &settingName));
  MOCK_CONST_METHOD0(getFacility, std::string());
};

class MockSettings : public ISettings {
public:
  virtual ~MockSettings() = default;

  MOCK_METHOD0(notifyApplySettings, void());
  MOCK_METHOD0(notifyCloseSettings, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE