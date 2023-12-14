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
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include "Analysis/IIndirectFitDataView.h"
#include "Analysis/IIndirectFitOutputOptionsModel.h"
#include "Analysis/IIndirectFitOutputOptionsView.h"
#include "Analysis/IIndirectFitPlotView.h"
#include "Analysis/IndirectDataAnalysisTab.h"
#include "IAddWorkspaceDialog.h"

#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces::IDA;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockIndirectDataAnalysisTab : public IIndirectDataAnalysisTab {
public:
  virtual ~MockIndirectDataAnalysisTab() = default;

  MOCK_METHOD1(handleDataAdded, void(MantidQt::CustomInterfaces::IDA::IAddWorkspaceDialog const *dialog));
  MOCK_METHOD0(handleDataChanged, void());
  MOCK_METHOD0(handleDataRemoved, void());
  MOCK_METHOD3(handleTableStartXChanged, void(double startX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex));
  MOCK_METHOD3(handleTableEndXChanged, void(double endX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex));

  MOCK_METHOD2(handleSingleFitClicked, void(WorkspaceID workspaceID, WorkspaceIndex workspaceIndex));
  MOCK_METHOD1(handleStartXChanged, void(double startX));
  MOCK_METHOD1(handleEndXChanged, void(double endX));
  MOCK_METHOD0(handlePlotSpectrumChanged, void());
  MOCK_METHOD1(handleFwhmChanged, void(double fwhm));
  MOCK_METHOD1(handleBackgroundChanged, void(double background));

  MOCK_METHOD0(handlePlotSelectedSpectra, void());
};

class MockIndirectFitPlotView : public IIndirectFitPlotView {
public:
  virtual ~MockIndirectFitPlotView() = default;

  MOCK_METHOD1(subscribePresenter, void(IIndirectFitPlotPresenter *presenter));

  MOCK_METHOD1(watchADS, void(bool watch));

  MOCK_CONST_METHOD0(getSelectedSpectrum, WorkspaceIndex());
  MOCK_CONST_METHOD0(getSelectedSpectrumIndex, FitDomainIndex());
  MOCK_CONST_METHOD0(getSelectedDataIndex, WorkspaceID());
  MOCK_CONST_METHOD0(dataSelectionSize, WorkspaceID());
  MOCK_CONST_METHOD0(isPlotGuessChecked, bool());

  MOCK_METHOD2(setAvailableSpectra, void(WorkspaceIndex minimum, WorkspaceIndex maximum));
  MOCK_METHOD2(setAvailableSpectra, void(std::vector<WorkspaceIndex>::const_iterator const &from,
                                         std::vector<WorkspaceIndex>::const_iterator const &to));

  MOCK_METHOD1(setMinimumSpectrum, void(int minimum));
  MOCK_METHOD1(setMaximumSpectrum, void(int maximum));
  MOCK_METHOD1(setPlotSpectrum, void(WorkspaceIndex spectrum));
  MOCK_METHOD1(appendToDataSelection, void(std::string const &dataName));
  MOCK_METHOD2(setNameInDataSelection, void(std::string const &dataName, WorkspaceID workspaceID));
  MOCK_METHOD0(clearDataSelection, void());

  MOCK_METHOD4(plotInTopPreview, void(QString const &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                      WorkspaceIndex spectrum, Qt::GlobalColor colour));
  MOCK_METHOD4(plotInBottomPreview, void(QString const &name, Mantid::API::MatrixWorkspace_sptr workspace,
                                         WorkspaceIndex spectrum, Qt::GlobalColor colour));

  MOCK_METHOD1(removeFromTopPreview, void(QString const &name));
  MOCK_METHOD1(removeFromBottomPreview, void(QString const &name));

  MOCK_METHOD1(enableFitSingleSpectrum, void(bool enable));
  MOCK_METHOD1(enablePlotGuess, void(bool enable));
  MOCK_METHOD1(enableSpectrumSelection, void(bool enable));
  MOCK_METHOD1(enableFitRangeSelection, void(bool enable));

  MOCK_METHOD1(setFitSingleSpectrumText, void(QString const &text));
  MOCK_METHOD1(setFitSingleSpectrumEnabled, void(bool enable));

  MOCK_METHOD1(setBackgroundLevel, void(double value));

  MOCK_METHOD2(setFitRange, void(double minimum, double maximum));
  MOCK_METHOD1(setFitRangeMinimum, void(double minimum));
  MOCK_METHOD1(setFitRangeMaximum, void(double maximum));
  MOCK_METHOD1(setFitRangeBounds, void(std::pair<double, double> const &bounds));

  MOCK_METHOD1(setBackgroundRangeVisible, void(bool visible));
  MOCK_METHOD1(setHWHMRangeVisible, void(bool visible));

  MOCK_METHOD1(allowRedraws, void(bool state));
  MOCK_METHOD0(redrawPlots, void());

  MOCK_CONST_METHOD1(displayMessage, void(std::string const &message));

  MOCK_METHOD1(setHWHMMinimum, void(double minimum));
  MOCK_METHOD1(setHWHMMaximum, void(double maximum));
  MOCK_METHOD2(setHWHMRange, void(double minimum, double maximum));

  MOCK_METHOD0(clearPreviews, void());
};

class MockIndirectFitOutputOptionsView : public IIndirectFitOutputOptionsView {
public:
  virtual ~MockIndirectFitOutputOptionsView() = default;

  MOCK_METHOD1(subscribePresenter, void(IIndirectFitOutputOptionsPresenter *presenter));

  MOCK_METHOD1(setGroupWorkspaceComboBoxVisible, void(bool visible));
  MOCK_METHOD1(setWorkspaceComboBoxVisible, void(bool visible));

  MOCK_METHOD0(clearPlotWorkspaces, void());
  MOCK_METHOD0(clearPlotTypes, void());
  MOCK_METHOD1(setAvailablePlotWorkspaces, void(std::vector<std::string> const &workspaceNames));
  MOCK_METHOD1(setAvailablePlotTypes, void(std::vector<std::string> const &parameterNames));

  MOCK_METHOD1(setPlotGroupWorkspaceIndex, void(int index));
  MOCK_METHOD1(setPlotWorkspacesIndex, void(int index));
  MOCK_METHOD1(setPlotTypeIndex, void(int index));

  MOCK_CONST_METHOD0(getSelectedGroupWorkspace, std::string());
  MOCK_CONST_METHOD0(getSelectedWorkspace, std::string());
  MOCK_CONST_METHOD0(getSelectedPlotType, std::string());

  MOCK_METHOD1(setPlotText, void(std::string const &text));
  MOCK_METHOD1(setSaveText, void(std::string const &text));

  MOCK_METHOD1(setPlotExtraOptionsEnabled, void(bool enable));
  MOCK_METHOD1(setPlotEnabled, void(bool enable));
  MOCK_METHOD1(setEditResultEnabled, void(bool enable));
  MOCK_METHOD1(setSaveEnabled, void(bool enable));

  MOCK_METHOD1(setEditResultVisible, void(bool visible));

  MOCK_METHOD1(displayWarning, void(std::string const &message));
};

class MockIndirectFitOutputOptionsModel : public IIndirectFitOutputOptionsModel {
public:
  virtual ~MockIndirectFitOutputOptionsModel() = default;

  MOCK_METHOD1(setResultWorkspace, void(WorkspaceGroup_sptr groupWorkspace));
  MOCK_METHOD1(setPDFWorkspace, void(WorkspaceGroup_sptr groupWorkspace));
  MOCK_CONST_METHOD0(getResultWorkspace, WorkspaceGroup_sptr());
  MOCK_CONST_METHOD0(getPDFWorkspace, WorkspaceGroup_sptr());

  MOCK_METHOD0(removePDFWorkspace, void());

  MOCK_CONST_METHOD1(isSelectedGroupPlottable, bool(std::string const &selectedGroup));
  MOCK_CONST_METHOD0(isResultGroupPlottable, bool());
  MOCK_CONST_METHOD0(isPDFGroupPlottable, bool());

  MOCK_METHOD0(clearSpectraToPlot, void());
  MOCK_CONST_METHOD0(getSpectraToPlot, std::vector<SpectrumToPlot>());

  MOCK_METHOD1(plotResult, void(std::string const &plotType));
  MOCK_METHOD2(plotPDF, void(std::string const &workspaceName, std::string const &plotType));

  MOCK_CONST_METHOD0(saveResult, void());

  MOCK_CONST_METHOD1(getWorkspaceParameters, std::vector<std::string>(std::string const &selectedGroup));
  MOCK_CONST_METHOD0(getPDFWorkspaceNames, std::vector<std::string>());

  MOCK_CONST_METHOD1(isResultGroupSelected, bool(std::string const &selectedGroup));

  MOCK_METHOD3(replaceFitResult,
               void(std::string const &inputName, std::string const &singleBinName, std::string const &outputName));
};

class MockIndirectFitDataModel : public IIndirectFitDataModel {
public:
  virtual ~MockIndirectFitDataModel() = default;

  MOCK_METHOD0(getFittingData, std::vector<IndirectFitData> *());
  MOCK_METHOD2(addWorkspace, void(const std::string &workspaceName, const std::string &spectra));
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

class MockFitDataView : public IIndirectFitDataView {
public:
  virtual ~MockFitDataView() = default;

  MOCK_METHOD1(subscribePresenter, void(IIndirectFitDataPresenter *presenter));

  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_METHOD1(validate, MantidQt::CustomInterfaces::UserInputValidator &(
                             MantidQt::CustomInterfaces::UserInputValidator &validator));
  MOCK_METHOD2(addTableEntry, void(size_t row, FitDataRow newRow));
  MOCK_METHOD3(updateNumCellEntry, void(double numEntry, size_t row, size_t column));
  MOCK_METHOD1(getColumnIndexFromName, int(QString ColName));
  MOCK_METHOD0(clearTable, void());
  MOCK_CONST_METHOD2(getText, QString(int row, int column));
  MOCK_CONST_METHOD0(getSelectedIndexes, QModelIndexList());

  MOCK_METHOD1(setSampleWSSuffices, void(const QStringList &suffices));
  MOCK_METHOD1(setSampleFBSuffices, void(const QStringList &suffices));
  MOCK_METHOD1(setResolutionWSSuffices, void(const QStringList &suffices));
  MOCK_METHOD1(setResolutionFBSuffices, void(const QStringList &suffices));

  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

GNU_DIAG_ON_SUGGEST_OVERRIDE