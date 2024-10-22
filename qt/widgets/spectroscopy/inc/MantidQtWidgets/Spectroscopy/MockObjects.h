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

#include "MantidQtWidgets/Spectroscopy/IDataModel.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputNameView.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsModel.h"
#include "MantidQtWidgets/Spectroscopy/OutputWidget/OutputPlotOptionsView.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/IRunSubscriber.h"
#include "MantidQtWidgets/Spectroscopy/RunWidget/RunView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsModel.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsView.h"

#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces;
using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace Mantid::API;

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

  MOCK_METHOD1(setLoadHistoryChecked, void(bool check));
  MOCK_CONST_METHOD0(isLoadHistoryChecked, bool());

  MOCK_METHOD1(setDeveloperFeatureFlags, void(QStringList const &flags));
  MOCK_CONST_METHOD0(developerFeatureFlags, QStringList());

  MOCK_METHOD3(setSetting, void(QString const &settingsGroup, QString const &settingName, bool const &value));
  MOCK_METHOD2(getSetting, QVariant(QString const &settingsGroup, QString const &settingName));

  MOCK_METHOD1(setApplyText, void(QString const &text));
  MOCK_METHOD1(setApplyEnabled, void(bool enable));
  MOCK_METHOD1(setOkEnabled, void(bool enable));
  MOCK_METHOD1(setCancelEnabled, void(bool enable));
};

class MockDataModel : public IDataModel {
public:
  virtual ~MockDataModel() = default;

  MOCK_METHOD0(getFittingData, std::vector<FitData> *());
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const FunctionModelSpectra &spectra));
  MOCK_METHOD2(addWorkspace, void(MatrixWorkspace_sptr workspace, const FunctionModelSpectra &spectra));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getWorkspace, MatrixWorkspace_sptr(FitDomainIndex index));
  MOCK_CONST_METHOD0(getWorkspaceNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(getNumberOfWorkspaces, WorkspaceID());
  MOCK_CONST_METHOD1(hasWorkspace, bool(std::string const &workspaceName));

  MOCK_METHOD2(setSpectra, void(const std::string &spectra, WorkspaceID workspaceID));
  MOCK_METHOD2(setSpectra, void(FunctionModelSpectra &&spectra, WorkspaceID workspaceID));
  MOCK_METHOD2(setSpectra, void(const FunctionModelSpectra &spectra, WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getSpectra, FunctionModelSpectra(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getDataset, FunctionModelDataset(WorkspaceID workspaceID));
  MOCK_CONST_METHOD1(getSpectrum, size_t(FitDomainIndex index));
  MOCK_CONST_METHOD1(getNumberOfSpectra, size_t(WorkspaceID workspaceID));

  MOCK_METHOD0(clear, void());

  MOCK_CONST_METHOD0(getNumberOfDomains, size_t());
  MOCK_CONST_METHOD2(getDomainIndex, FitDomainIndex(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getSubIndices, std::pair<WorkspaceID, WorkspaceIndex>(FitDomainIndex));

  MOCK_CONST_METHOD0(getQValuesForData, std::vector<double>());
  MOCK_CONST_METHOD0(getResolutionsForFit, std::vector<std::pair<std::string, size_t>>());
  MOCK_CONST_METHOD1(createDisplayName, std::string(WorkspaceID workspaceID));

  MOCK_METHOD1(removeWorkspace, void(WorkspaceID workspaceID));
  MOCK_METHOD1(removeDataByIndex, void(FitDomainIndex fitDomainIndex));

  MOCK_METHOD3(setStartX, void(double startX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setStartX, void(double startX, WorkspaceID workspaceID));
  MOCK_METHOD2(setStartX, void(double startX, FitDomainIndex fitDomainIndex));
  MOCK_METHOD3(setEndX, void(double endX, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setEndX, void(double endX, WorkspaceID workspaceID));
  MOCK_METHOD2(setEndX, void(double endX, FitDomainIndex fitDomainIndex));
  MOCK_METHOD3(setExcludeRegion, void(const std::string &exclude, WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_METHOD2(setExcludeRegion, void(const std::string &exclude, FitDomainIndex index));
  MOCK_METHOD1(removeSpecialValues, void(const std::string &name));
  MOCK_METHOD1(setResolution, bool(const std::string &name));
  MOCK_METHOD2(setResolution, bool(const std::string &name, WorkspaceID workspaceID));
  MOCK_CONST_METHOD2(getFittingRange, std::pair<double, double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getFittingRange, std::pair<double, double>(FitDomainIndex index));
  MOCK_CONST_METHOD2(getExcludeRegion, std::string(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getExcludeRegion, std::string(FitDomainIndex index));
  MOCK_CONST_METHOD2(getExcludeRegionVector, std::vector<double>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getExcludeRegionVector, std::vector<double>(FitDomainIndex index));
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

class MockRunView : public IRunView {
public:
  virtual ~MockRunView() = default;

  MOCK_METHOD1(subscribePresenter, void(IRunPresenter *presenter));

  MOCK_METHOD1(setRunText, void(std::string const &text));

  MOCK_METHOD1(displayWarning, void(std::string const &message));
};

class MockRunSubscriber : public IRunSubscriber {
public:
  virtual ~MockRunSubscriber() = default;

  MOCK_CONST_METHOD1(handleValidation, void(IUserInputValidator *validator));
  MOCK_METHOD0(handleRun, void());
  MOCK_CONST_METHOD0(getSubscriberName, const std::string());
};


class MockOutputNameView : public IOutputNameView {
public:
  virtual ~MockOutputNameView() = default;
  MOCK_METHOD1(subscribePresenter, void(IOutputNamePresenter *presenter));
  MOCK_CONST_METHOD0(enableLabelEditor, void());
  MOCK_CONST_METHOD2(setWarningLabel, void(std::string const &text, std::string const &textColor));
  MOCK_CONST_METHOD1(setOutputNameLabel, void(std::string const &text));
  MOCK_CONST_METHOD0(getCurrentLabel, std::string());
  MOCK_CONST_METHOD0(getCurrentOutputName, std::string());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE