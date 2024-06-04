// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "Manipulation/DataManipulation.h"
#include "Manipulation/ElwinModel.h"
#include "Manipulation/ElwinPresenter.h"
#include "Manipulation/ElwinView.h"
#include "Manipulation/IElwinView.h"
#include "QENSFitting/FitOutput.h"
#include "QENSFitting/FitPlotModel.h"
#include "QENSFitting/FitTab.h"
#include "QENSFitting/FunctionBrowser/FunctionTemplateView.h"
#include "QENSFitting/FunctionBrowser/ITemplatePresenter.h"
#include "QENSFitting/IFitDataView.h"
#include "QENSFitting/IFitOutputOptionsModel.h"
#include "QENSFitting/IFitOutputOptionsView.h"
#include "QENSFitting/IFitPlotView.h"
#include "QENSFitting/IFittingModel.h"
#include "QENSFitting/InelasticFitPropertyBrowser.h"

#include "MantidAPI/IFunction_fwd.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidQtWidgets/Common/FunctionModelDataset.h"
#include "MantidQtWidgets/Common/FunctionModelSpectra.h"
#include "MantidQtWidgets/Common/IAddWorkspaceDialog.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces::Inelastic;
using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockFitTab : public IFitTab {
public:
  virtual ~MockFitTab() = default;

  MOCK_CONST_METHOD0(tabName, std::string());
  MOCK_METHOD1(handleDataAdded, void(MantidQt::MantidWidgets::IAddWorkspaceDialog const *dialog));
  MOCK_METHOD0(handleDataChanged, void());
  MOCK_METHOD0(handleDataRemoved, void());
  MOCK_METHOD3(handleTableStartXChanged, void(double startX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex));
  MOCK_METHOD3(handleTableEndXChanged, void(double endX, WorkspaceID workspaceID, WorkspaceIndex workspaceIndex));
  MOCK_METHOD1(handleFunctionListChanged, void(const std::map<std::string, std::string> &functionStrings));

  MOCK_METHOD0(handleSingleFitClicked, void());
  MOCK_METHOD1(handleStartXChanged, void(double startX));
  MOCK_METHOD1(handleEndXChanged, void(double endX));
  MOCK_METHOD0(handlePlotSpectrumChanged, void());
  MOCK_METHOD1(handleFwhmChanged, void(double fwhm));
  MOCK_METHOD1(handleBackgroundChanged, void(double background));

  MOCK_METHOD0(handlePlotSelectedSpectra, void());

  MOCK_METHOD0(handleFunctionChanged, void());
  MOCK_METHOD1(handleFitComplete, void(bool const error));
};

class MockFitPlotModel : public IFitPlotModel {
public:
  virtual ~MockFitPlotModel() = default;

  MOCK_CONST_METHOD0(getWorkspace, Mantid::API::MatrixWorkspace_sptr());
  MOCK_CONST_METHOD0(getResultWorkspace, Mantid::API::MatrixWorkspace_sptr());
  MOCK_CONST_METHOD0(getGuessWorkspace, Mantid::API::MatrixWorkspace_sptr());
  MOCK_CONST_METHOD1(getSpectra, MantidQt::MantidWidgets::FunctionModelSpectra(WorkspaceID workspaceID));

  MOCK_CONST_METHOD0(getActiveWorkspaceID, WorkspaceID());
  MOCK_CONST_METHOD0(getActiveWorkspaceIndex, WorkspaceIndex());
  MOCK_CONST_METHOD0(getActiveDomainIndex, FitDomainIndex());
  MOCK_CONST_METHOD0(numberOfWorkspaces, WorkspaceID());

  MOCK_CONST_METHOD0(getRange, std::pair<double, double>());
  MOCK_CONST_METHOD0(getWorkspaceRange, std::pair<double, double>());
  MOCK_CONST_METHOD0(getResultRange, std::pair<double, double>());
  MOCK_CONST_METHOD0(getFirstHWHM, std::optional<double>());
  MOCK_CONST_METHOD0(getFirstPeakCentre, std::optional<double>());
  MOCK_CONST_METHOD0(getFirstBackgroundLevel, std::optional<double>());
  MOCK_CONST_METHOD1(calculateHWHMMaximum, double(double maximum));
  MOCK_CONST_METHOD1(calculateHWHMMinimum, double(double minimum));
  MOCK_CONST_METHOD0(canCalculateGuess, bool());

  MOCK_METHOD1(setActiveIndex, void(WorkspaceID workspaceID));
  MOCK_METHOD1(setActiveSpectrum, void(WorkspaceIndex spectrum));

  MOCK_METHOD1(setFittingData, void(std::vector<FitData> *fittingData));
  MOCK_METHOD1(setFitOutput, void(IFitOutput *fitOutput));
  MOCK_METHOD1(setFitFunction, void(Mantid::API::MultiDomainFunction_sptr function));
};

class MockFitPlotView : public IFitPlotView {
public:
  virtual ~MockFitPlotView() = default;

  MOCK_METHOD1(subscribePresenter, void(IFitPlotPresenter *presenter));

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

class MockFitOutputOptionsView : public IFitOutputOptionsView {
public:
  virtual ~MockFitOutputOptionsView() = default;

  MOCK_METHOD1(subscribePresenter, void(IFitOutputOptionsPresenter *presenter));

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

class MockFitOutputOptionsModel : public IFitOutputOptionsModel {
public:
  virtual ~MockFitOutputOptionsModel() = default;

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

class MockFittingModel : public IFittingModel {
public:
  MOCK_CONST_METHOD2(isPreviouslyFit, bool(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD0(isInvalidFunction, std::optional<std::string>());
  MOCK_CONST_METHOD0(getFitParameterNames, std::vector<std::string>());
  MOCK_CONST_METHOD0(getFitFunction, Mantid::API::MultiDomainFunction_sptr());
  MOCK_CONST_METHOD2(getParameterValues,
                     std::unordered_map<std::string, ParameterValue>(WorkspaceID workspaceID, WorkspaceIndex spectrum));

  MOCK_METHOD1(setFitFunction, void(Mantid::API::MultiDomainFunction_sptr function));
  MOCK_METHOD2(setFWHM, void(double fwhm, WorkspaceID WorkspaceID));
  MOCK_METHOD2(setBackground, void(double fwhm, WorkspaceID WorkspaceID));
  MOCK_METHOD3(setDefaultParameterValue, void(const std::string &name, double value, WorkspaceID workspaceID));

  MOCK_CONST_METHOD2(getFitParameters,
                     std::unordered_map<std::string, ParameterValue>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD1(getDefaultParameters, std::unordered_map<std::string, ParameterValue>(WorkspaceID workspaceID));

  MOCK_CONST_METHOD1(validate, void(MantidQt::CustomInterfaces::UserInputValidator &validator));

  MOCK_METHOD0(clearWorkspaces, void());
  MOCK_CONST_METHOD1(getWorkspace, Mantid::API::MatrixWorkspace_sptr(WorkspaceID workspaceID));
  MOCK_CONST_METHOD0(getNumberOfWorkspaces, WorkspaceID());
  MOCK_CONST_METHOD0(isMultiFit, bool());

  MOCK_METHOD1(addOutput, void(Mantid::API::IAlgorithm_sptr fitAlgorithm));
  MOCK_CONST_METHOD0(getFitOutput, IFitOutput *());

  MOCK_METHOD1(setFittingMode, void(FittingMode mode));
  MOCK_CONST_METHOD0(getFittingMode, FittingMode());

  MOCK_METHOD0(updateFitTypeString, void());
  MOCK_CONST_METHOD2(getResultLocation,
                     std::optional<ResultLocationNew>(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD0(getResultWorkspace, Mantid::API::WorkspaceGroup_sptr());
  MOCK_CONST_METHOD0(getResultGroup, Mantid::API::WorkspaceGroup_sptr());
  MOCK_CONST_METHOD1(getFittingAlgorithm, Mantid::API::IAlgorithm_sptr(FittingMode mode));
  MOCK_CONST_METHOD0(getSingleFittingAlgorithm, Mantid::API::IAlgorithm_sptr());
  MOCK_CONST_METHOD2(getSingleFunction, Mantid::API::IFunction_sptr(WorkspaceID workspaceID, WorkspaceIndex spectrum));
  MOCK_CONST_METHOD0(getOutputBasename, std::string());

  MOCK_METHOD1(cleanFailedRun, void(const Mantid::API::IAlgorithm_sptr &fittingAlgorithm));
  MOCK_METHOD0(removeFittingData, void());
  MOCK_METHOD0(addDefaultParameters, void());
  MOCK_METHOD0(removeDefaultParameters, void());
  MOCK_CONST_METHOD0(getFitDataModel, IFitDataModel *());
  MOCK_CONST_METHOD0(getFitPlotModel, IFitPlotModel *());
};

class MockFitDataModel : public IFitDataModel {
public:
  virtual ~MockFitDataModel() = default;

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

class MockFitDataView : public IFitDataView {
public:
  virtual ~MockFitDataView() = default;

  MOCK_METHOD1(subscribePresenter, void(IFitDataPresenter *presenter));

  MOCK_CONST_METHOD0(getDataTable, QTableWidget *());
  MOCK_CONST_METHOD0(isTableEmpty, bool());
  MOCK_METHOD1(validate, void(MantidQt::CustomInterfaces::UserInputValidator &validator));
  MOCK_METHOD2(addTableEntry, void(size_t row, FitDataRow newRow));
  MOCK_METHOD3(updateNumCellEntry, void(double numEntry, size_t row, size_t column));
  MOCK_METHOD1(getColumnIndexFromName, int(std::string const &ColName));
  MOCK_METHOD0(clearTable, void());
  MOCK_CONST_METHOD2(getText, QString(int row, int column));
  MOCK_CONST_METHOD0(getSelectedIndexes, QModelIndexList());
  MOCK_CONST_METHOD1(dataColumnContainsText, bool(const std::string &columnText));

  MOCK_METHOD1(setSampleWSSuffices, void(const QStringList &suffices));
  MOCK_METHOD1(setSampleFBSuffices, void(const QStringList &suffices));
  MOCK_METHOD1(setResolutionWSSuffices, void(const QStringList &suffices));
  MOCK_METHOD1(setResolutionFBSuffices, void(const QStringList &suffices));

  MOCK_METHOD1(displayWarning, void(std::string const &warning));
};

class MockFunctionTemplateView : public FunctionTemplateView {
public:
  virtual ~MockFunctionTemplateView() = default;

  MOCK_METHOD1(setFunction, void(std::string const &funStr));
  MOCK_CONST_METHOD0(getGlobalFunction, IFunction_sptr());
  MOCK_CONST_METHOD0(getFunction, IFunction_sptr());
  MOCK_METHOD1(setNumberOfDatasets, void(int));
  MOCK_CONST_METHOD0(getNumberOfDatasets, int());
  MOCK_METHOD1(setDatasets, void(const QList<FunctionModelDataset> &datasets));
  MOCK_CONST_METHOD0(getGlobalParameters, std::vector<std::string>());
  MOCK_CONST_METHOD0(getLocalParameters, std::vector<std::string>());
  MOCK_METHOD1(setGlobalParameters, void(std::vector<std::string> const &globals));
  MOCK_METHOD1(updateMultiDatasetParameters, void(const IFunction &fun));
  MOCK_METHOD1(setCurrentDataset, void(int i));
  MOCK_METHOD0(getCurrentDataset, int());
  MOCK_METHOD1(updateParameterNames, void(const std::map<int, std::string> &parameterNames));
  MOCK_METHOD1(setGlobalParametersQuiet, void(std::vector<std::string> const &globals));
  MOCK_METHOD1(setErrorsEnabled, void(bool enabled));
  MOCK_METHOD1(setBackgroundA0, void(double value));
  MOCK_METHOD2(setResolution, void(std::string const &name, WorkspaceID const &index));
  MOCK_METHOD1(setResolution, void(const std::vector<std::pair<std::string, size_t>> &fitResolutions));
  MOCK_METHOD1(setQValues, void(const std::vector<double> &qValues));
  // protected Slots
  MOCK_METHOD1(parameterChanged, void(QtProperty *));
  MOCK_METHOD1(parameterButtonClicked, void(QtProperty *));
  // Private methods
  MOCK_METHOD0(createProperties, void());
};

class MockFunctionTemplatePresenter : public ITemplatePresenter {
public:
  MockFunctionTemplatePresenter(MockFunctionTemplateView *view) : m_view(view) {}
  virtual ~MockFunctionTemplatePresenter() = default;

  FunctionTemplateView *browser() override { return m_view; }

  MOCK_METHOD0(init, void());
  MOCK_METHOD1(updateAvailableFunctions, void(const std::map<std::string, std::string> &functionInitialisationStrings));

  MOCK_METHOD1(setNumberOfDatasets, void(int));
  MOCK_CONST_METHOD0(getNumberOfDatasets, int());
  MOCK_METHOD0(getCurrentDataset, int());

  MOCK_METHOD1(setFitType, void(std::string const &name));

  MOCK_METHOD1(setFunction, void(std::string const &funStr));
  MOCK_CONST_METHOD0(getGlobalFunction, Mantid::API::IFunction_sptr());
  MOCK_CONST_METHOD0(getFunction, Mantid::API::IFunction_sptr());

  MOCK_CONST_METHOD0(getGlobalParameters, std::vector<std::string>());
  MOCK_CONST_METHOD0(getLocalParameters, std::vector<std::string>());
  MOCK_METHOD1(setGlobalParameters, void(std::vector<std::string> const &globals));
  MOCK_METHOD2(setGlobal, void(std::string const &parameterName, bool on));

  MOCK_METHOD1(updateMultiDatasetParameters, void(const Mantid::API::IFunction &fun));
  MOCK_METHOD1(updateMultiDatasetParameters, void(const Mantid::API::ITableWorkspace &table));
  MOCK_METHOD1(updateParameters, void(const Mantid::API::IFunction &fun));

  MOCK_METHOD1(setCurrentDataset, void(int i));
  MOCK_METHOD1(setDatasets, void(const QList<MantidQt::MantidWidgets::FunctionModelDataset> &datasets));

  MOCK_CONST_METHOD0(getEstimationDataSelector, EstimationDataSelector());
  MOCK_METHOD1(updateParameterEstimationData, void(DataForParameterEstimationCollection &&data));
  MOCK_METHOD0(estimateFunctionParameters, void());

  MOCK_METHOD1(setErrorsEnabled, void(bool enabled));

  MOCK_METHOD1(setNumberOfExponentials, void(int nExponentials));
  MOCK_METHOD1(setStretchExponential, void(bool on));
  MOCK_METHOD1(setBackground, void(std::string const &name));
  MOCK_METHOD1(tieIntensities, void(bool on));
  MOCK_CONST_METHOD0(canTieIntensities, bool());

  MOCK_METHOD2(setSubType, void(std::size_t subTypeIndex, int typeIndex));
  MOCK_METHOD1(setDeltaFunction, void(bool on));
  MOCK_METHOD1(setTempCorrection, void(bool on));
  MOCK_METHOD1(setBackgroundA0, void(double value));
  MOCK_METHOD1(setResolution, void(const std::vector<std::pair<std::string, size_t>> &fitResolutions));
  MOCK_METHOD1(setQValues, void(const std::vector<double> &qValues));

  MOCK_METHOD1(handleEditLocalParameter, void(std::string const &parameterName));
  MOCK_METHOD2(handleParameterValueChanged, void(std::string const &parameterName, double value));
  MOCK_METHOD5(handleEditLocalParameterFinished,
               void(std::string const &parameterName, QList<double> const &values, QList<bool> const &fixes,
                    QStringList const &ties, QStringList const &constraints));

private:
  FunctionTemplateView *m_view;
};

class MockInelasticFitPropertyBrowser : public IInelasticFitPropertyBrowser {
public:
  virtual ~MockInelasticFitPropertyBrowser() = default;

  MOCK_METHOD1(subscribePresenter, void(IFittingPresenter *presenter));
  MOCK_CONST_METHOD0(getFitFunction, Mantid::API::MultiDomainFunction_sptr());
  MOCK_CONST_METHOD1(minimizer, std::string(bool withProperties));
  MOCK_CONST_METHOD1(fitProperties,
                     std::unique_ptr<Mantid::API::AlgorithmRuntimeProps>(FittingMode const &fittingMode));
  MOCK_METHOD1(setFitEnabled, void(bool enable));
  MOCK_METHOD1(setCurrentDataset, void(FitDomainIndex i));
  MOCK_METHOD1(setErrorsEnabled, void(bool enabled));
  MOCK_METHOD1(setBackgroundA0, void(double value));
  MOCK_CONST_METHOD0(getEstimationDataSelector, EstimationDataSelector());
  MOCK_METHOD1(updateParameterEstimationData, void(DataForParameterEstimationCollection &&data));
  MOCK_METHOD0(estimateFunctionParameters, void());
  MOCK_CONST_METHOD0(getFittingMode, FittingMode());
  MOCK_METHOD1(updateParameters, void(const IFunction &fun));
  MOCK_METHOD1(updateMultiDatasetParameters, void(const IFunction &fun));
  MOCK_METHOD1(updateMultiDatasetParameters, void(const ITableWorkspace &params));
  MOCK_METHOD1(updateFunctionListInBrowser, void(const std::map<std::string, std::string> &functionStrings));
  MOCK_METHOD4(updateFunctionBrowserData,
               void(int nData, const QList<FunctionModelDataset> &datasets, const std::vector<double> &qValues,
                    const std::vector<std::pair<std::string, size_t>> &fitResolutions));
  MOCK_METHOD2(updateFitStatusData,
               void(const std::vector<std::string> &status, const std::vector<double> &chiSquared));
};

class MockElwinView : public IElwinView {
public:
  virtual ~MockElwinView() = default;

  MOCK_METHOD1(subscribePresenter, void(IElwinPresenter *presenter));
  MOCK_METHOD0(setup, void());
  MOCK_CONST_METHOD0(getPlotOptions, IOutputPlotOptionsView *());

  MOCK_METHOD2(setAvailableSpectra,
               void(MantidQt::MantidWidgets::WorkspaceIndex minimum, MantidQt::MantidWidgets::WorkspaceIndex maximum));
  MOCK_METHOD2(setAvailableSpectra,
               void(const std::vector<MantidQt::MantidWidgets::WorkspaceIndex>::const_iterator &from,
                    const std::vector<MantidQt::MantidWidgets::WorkspaceIndex>::const_iterator &to));
  MOCK_METHOD2(plotInput, void(Mantid::API::MatrixWorkspace_sptr inputWS, int spectrum));
  MOCK_METHOD1(newInputDataFromDialog, void(std::vector<std::string> const &names));
  MOCK_METHOD0(clearPreviewFile, void());

  MOCK_METHOD1(setRunIsRunning, void(const bool running));
  MOCK_METHOD1(setSaveResultEnabled, void(const bool enabled));
  MOCK_CONST_METHOD0(getPreviewSpec, int());

  MOCK_CONST_METHOD1(getPreviewWorkspaceName, std::string(int index));
  MOCK_CONST_METHOD1(getPreviewFilename, std::string(int index));
  MOCK_CONST_METHOD0(getCurrentPreview, std::string());

  MOCK_METHOD0(clearDataTable, void());
  MOCK_METHOD3(addTableEntry, void(int row, std::string const &name, std::string const &wsIndexes));

  MOCK_METHOD0(getSelectedData, QModelIndexList());
  MOCK_METHOD0(selectAllRows, void());

  MOCK_CONST_METHOD0(isGroupInput, bool());
  MOCK_CONST_METHOD0(isRowCollapsed, bool());
  MOCK_CONST_METHOD0(isTableEmpty, bool());

  MOCK_METHOD0(getNormalise, bool());
  MOCK_METHOD0(getBackgroundSubtraction, bool());
  MOCK_METHOD0(getLogName, std::string());
  MOCK_METHOD0(getLogValue, std::string());

  MOCK_METHOD1(setIntegrationStart, void(double value));
  MOCK_METHOD1(setIntegrationEnd, void(double value));
  MOCK_METHOD1(setBackgroundStart, void(double value));
  MOCK_METHOD1(setBackgroundEnd, void(double value));

  MOCK_METHOD0(getIntegrationStart, double());
  MOCK_METHOD0(getIntegrationEnd, double());
  MOCK_METHOD0(getBackgroundStart, double());
  MOCK_METHOD0(getBackgroundEnd, double());

  MOCK_CONST_METHOD1(showMessageBox, void(std::string const &message));
};

class MockElwinModel : public IElwinModel {
public:
  virtual ~MockElwinModel() = default;

  MOCK_METHOD3(setupLoadAlgorithm, void(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner,
                                        std::string const &filepath, std::string const &outputName));
  MOCK_METHOD2(createGroupedWorkspaces,
               std::string(MatrixWorkspace_sptr workspce, FunctionModelSpectra const &spectra));
  MOCK_METHOD3(setupGroupAlgorithm,
               void(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &inputWorkspacesString,
                    std::string const &inputGroupWsName));
  MOCK_METHOD5(setupElasticWindowMultiple,
               void(MantidQt::API::BatchAlgorithmRunner *batchAlgoRunner, std::string const &workspaceBaseName,
                    std::string const &inputGroupWsName, std::string const &sampleEnvironmentLogName,
                    std::string const &sampleEnvironmentLogValue));
  MOCK_CONST_METHOD1(ungroupAlgorithm, void(std::string const &inputWorkspaces));
  MOCK_CONST_METHOD2(groupAlgorithm, void(std::string const &inputWorkspaces, std::string const &outputWorkspace));
  MOCK_METHOD1(setIntegrationStart, void(double integrationStart));
  MOCK_METHOD1(setIntegrationEnd, void(double integrationEnd));
  MOCK_METHOD1(setBackgroundStart, void(double backgroundStart));
  MOCK_METHOD1(setBackgroundEnd, void(double backgroundEnd));
  MOCK_METHOD1(setBackgroundSubtraction, void(bool backgroundSubstraction));
  MOCK_METHOD1(setNormalise, void(bool normalise));
  MOCK_METHOD1(setOutputWorkspaceNames, void(std::string const &workspaceBaseName));
  MOCK_CONST_METHOD0(getOutputWorkspaceNames, std::string());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE