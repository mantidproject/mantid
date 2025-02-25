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

#include "Reduction/DataReduction.h"
#include "Reduction/ISISEnergyTransferModel.h"
#include "Reduction/ISISEnergyTransferView.h"

#include <string>
#include <utility>
#include <vector>

using namespace MantidQt::CustomInterfaces;

GNU_DIAG_OFF_SUGGEST_OVERRIDE

class MockDataReduction : public IDataReduction {
public:
  virtual ~MockDataReduction() = default;

  MOCK_METHOD0(instrumentWorkspace, Mantid::API::MatrixWorkspace_sptr());

  MOCK_CONST_METHOD0(getInstrumentConfiguration, MantidQt::MantidWidgets::IInstrumentConfig *());
  MOCK_METHOD0(getInstrumentDetails, QMap<QString, QString>());

  MOCK_METHOD1(showAnalyserAndReflectionOptions, void(bool visible));
};

class MockIETModel : public IIETModel {
public:
  virtual ~MockIETModel() = default;

  MOCK_METHOD2(setInstrumentProperties, void(IAlgorithmRuntimeProps &properties, InstrumentData const &instData));

  MOCK_METHOD1(validateRunData, std::vector<std::string>(IETRunData const &runData));
  MOCK_METHOD1(validatePlotData, std::vector<std::string>(IETPlotData const &plotData));

  MOCK_METHOD4(energyTransferAlgorithm,
               MantidQt::API::IConfiguredAlgorithm_sptr(InstrumentData const &instData, IETRunData &runParams,
                                                        std::string const &outputGroupName,
                                                        std::string const &outputLabel));
  MOCK_CONST_METHOD2(plotRawAlgorithmQueue,
                     std::deque<MantidQt::API::IConfiguredAlgorithm_sptr>(InstrumentData const &instData,
                                                                          IETPlotData const &plotData));

  MOCK_METHOD2(saveWorkspace, void(std::string const &workspaceName, IETSaveData const &saveData));

  MOCK_METHOD4(createGroupingWorkspace, void(std::string const &instrumentName, std::string const &analyser,
                                             std::string const &customGrouping, std::string const &outputName));

  MOCK_METHOD1(loadDetailedBalance, double(std::string const &filename));

  MOCK_METHOD4(groupWorkspaces, std::vector<std::string>(std::string const &groupName, std::string const &instrument,
                                                         std::string const &groupOption, bool const shouldGroup));

  MOCK_CONST_METHOD2(getOutputGroupName, std::string(InstrumentData const &instData, std::string const &inputFiles));
  MOCK_CONST_METHOD0(outputGroupName, std::string());
  MOCK_CONST_METHOD0(outputWorkspaceNames, std::vector<std::string>());
};

class MockIETView : public IIETView {
public:
  virtual ~MockIETView() = default;

  MOCK_METHOD1(subscribePresenter, void(IIETPresenter *presenter));

  MOCK_CONST_METHOD0(getRunData, IETRunData());
  MOCK_CONST_METHOD0(getPlotData, IETPlotData());
  MOCK_CONST_METHOD0(getSaveData, IETSaveData());
  MOCK_CONST_METHOD0(getGroupOutputOption, std::string());
  MOCK_CONST_METHOD0(getRunView, IRunView *());
  MOCK_CONST_METHOD0(getOutputName, IOutputNameView *());
  MOCK_CONST_METHOD0(getPlotOptionsView, IOutputPlotOptionsView *());
  MOCK_CONST_METHOD0(getGroupOutputCheckbox, bool());
  MOCK_CONST_METHOD0(getFirstFilename, std::string());
  MOCK_CONST_METHOD0(getInputText, std::string());
  MOCK_CONST_METHOD0(isRunFilesValid, bool());
  MOCK_CONST_METHOD1(validateCalibrationFileType, void(IUserInputValidator *uiv));
  MOCK_CONST_METHOD1(validateRebinString, void(IUserInputValidator *uiv));
  MOCK_CONST_METHOD2(validateGroupingProperties,
                     std::optional<std::string>(std::size_t const &spectraMin, std::size_t const &spectraMax));
  MOCK_CONST_METHOD0(showRebinWidthPrompt, bool());
  MOCK_CONST_METHOD3(showSaveCustomGroupingDialog,
                     void(std::string const &customGroupingOutput, std::string const &defaultGroupingFilename,
                          std::string const &saveDirectory));
  MOCK_CONST_METHOD1(displayWarning, void(std::string const &message));
  MOCK_METHOD1(setCalibVisible, void(bool visible));
  MOCK_METHOD1(setEfixedVisible, void(bool visible));
  MOCK_METHOD1(setBackgroundSectionVisible, void(bool visible));
  MOCK_METHOD1(setPlotTimeSectionVisible, void(bool visible));
  MOCK_METHOD1(setAnalysisSectionVisible, void(bool visible));
  MOCK_METHOD1(setPlottingOptionsVisible, void(bool visible));
  MOCK_METHOD1(setAclimaxSaveVisible, void(bool visible));
  MOCK_METHOD1(setSPEVisible, void(bool visible));
  MOCK_METHOD1(setFoldMultipleFramesVisible, void(bool visible));
  MOCK_METHOD1(setOutputInCm1Visible, void(bool visible));
  MOCK_METHOD1(setGroupOutputCheckBoxVisible, void(bool visible));
  MOCK_METHOD1(setGroupOutputDropdownVisible, void(bool visible));
  MOCK_METHOD1(setDetailedBalance, void(double detailedBalance));
  MOCK_METHOD1(setRunFilesEnabled, void(bool enable));
  MOCK_METHOD1(setSingleRebin, void(bool enable));
  MOCK_METHOD1(setMultipleRebin, void(bool enable));
  MOCK_METHOD1(setSaveEnabled, void(bool enable));
  MOCK_METHOD1(setPlotTimeIsPlotting, void(bool plotting));
  MOCK_METHOD2(setFileExtensionsByName, void(QStringList calibrationFbSuffixes, QStringList calibrationWSSuffixes));
  MOCK_METHOD1(setLoadHistory, void(bool doLoadHistory));
  MOCK_METHOD1(setRunButtonText, void(std::string const &runText));
  MOCK_METHOD1(setEnableOutputOptions, void(bool const enable));

  MOCK_METHOD2(setInstrumentSpectraRange, void(int specMin, int specMax));
  MOCK_METHOD4(setInstrumentRebinning,
               void(std::vector<double> const &rebinParams, std::string const &rebinText, bool checked, int tabIndex));
  MOCK_METHOD2(setInstrumentEFixed, void(std::string const &instrumentName, double eFixed));
  MOCK_METHOD1(setInstrumentGrouping, void(std::string const &instrumentName));
  MOCK_METHOD1(setInstrumentSpecDefault, void(std::map<std::string, bool> &specMap));

  MOCK_METHOD1(showMessageBox, void(std::string const &message));

  MOCK_CONST_METHOD0(hideOutputNameBox, void());
};

GNU_DIAG_ON_SUGGEST_OVERRIDE
