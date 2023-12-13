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

#include "Analysis/IIndirectFitOutputOptionsModel.h"
#include "Analysis/IIndirectFitOutputOptionsView.h"
#include "Analysis/IndirectDataAnalysisTab.h"

using namespace MantidQt::CustomInterfaces::IDA;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockIndirectDataAnalysisTab : public IIndirectDataAnalysisTab {
public:
  MOCK_METHOD2(handleSingleFitClicked, void(WorkspaceID workspaceID, WorkspaceIndex workspaceIndex));
  MOCK_METHOD1(handleStartXChanged, void(double startX));
  MOCK_METHOD1(handleEndXChanged, void(double endX));
  MOCK_METHOD0(handlePlotSpectrumChanged, void());
  MOCK_METHOD1(handleFwhmChanged, void(double fwhm));
  MOCK_METHOD1(handleBackgroundChanged, void(double background));

  MOCK_METHOD0(handlePlotSelectedSpectra, void());
};

class MockIndirectFitPlotView final : public IIndirectFitPlotView {
public:
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

class MockIndirectFitOutputOptionsView final : public IIndirectFitOutputOptionsView {
public:
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

GNU_DIAG_ON_SUGGEST_OVERRIDE