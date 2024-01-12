// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISEnergyTransferPresenter.h"
#include "Common/SettingsHelper.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferView.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <filesystem>

using namespace Mantid::API;
using MantidQt::API::BatchAlgorithmRunner;

namespace {
bool doesExistInADS(std::string const &workspaceName) {
  return AnalysisDataService::Instance().doesExist(workspaceName);
}
} // namespace

namespace MantidQt::CustomInterfaces {

IETPresenter::IETPresenter(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent), m_model(std::make_unique<IETModel>()),
      m_view(std::make_unique<IETView>(this, parent)) {

  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_view->getPlotOptionsView(), PlotWidget::SpectraSlice));

  connect(this, SIGNAL(newInstrumentConfiguration()), this, SLOT(setInstrumentDefault()));

  connect(this, SIGNAL(updateRunButton(bool, std::string const &, QString const &, QString const &)), m_view.get(),
          SLOT(updateRunButton(bool, std::string const &, QString const &, QString const &)));
}

IETPresenter::~IETPresenter() = default;

void IETPresenter::setup() {}

bool IETPresenter::validateInstrumentDetails() {
  auto const instrument = getInstrumentName().toStdString();
  if (instrument.empty()) {
    showMessageBox("Please select a valid facility and/or instrument.");
    return false;
  }

  QMap<QString, QString> instrumentDetails = getInstrumentDetails();
  std::set<std::string> keys = {"spectra-min", "spectra-max"};
  for (const auto &key : keys) {
    if (!instrumentDetails.contains(QString::fromStdString(key)) ||
        instrumentDetails[QString::fromStdString(key)].isEmpty()) {
      showMessageBox(QString::fromStdString("Could not find " + key + " for the " + instrument +
                                            " instrument. Please select a valid instrument."));
      return false;
    }
  }

  return true;
}

InstrumentData IETPresenter::getInstrumentData() {
  QMap<QString, QString> instrumentDetails = getInstrumentDetails();

  return InstrumentData(
      getInstrumentName().toStdString(), getAnalyserName().toStdString(), getReflectionName().toStdString(),
      instrumentDetails["spectra-min"].toInt(), instrumentDetails["spectra-max"].toInt(),
      instrumentDetails["Efixed"].toDouble(), instrumentDetails["rebin-default"].toStdString(),
      instrumentDetails["cm-1-convert-choice"] == "true", instrumentDetails["save-nexus-choice"] == "true",
      instrumentDetails["save-ascii-choice"] == "true", instrumentDetails["fold-frames-choice"] == "true");
}

void IETPresenter::setInstrumentDefault() {
  if (validateInstrumentDetails()) {
    InstrumentData instrumentDetails = getInstrumentData();
    m_view->setInstrumentDefault(instrumentDetails);
    bool const iris = instrumentDetails.getInstrument() == "IRIS";
    bool const osiris = instrumentDetails.getInstrument() == "OSIRIS";
    bool const irisOrOsiris =
        instrumentDetails.getInstrument() == "OSIRIS" || instrumentDetails.getInstrument() == "IRIS";
    bool const toscaOrTfxa =
        instrumentDetails.getInstrument() == "TOSCA" || instrumentDetails.getInstrument() == "TFXA";

    m_view->setGroupOutputCheckBoxVisible(osiris);
    m_view->setGroupOutputDropdownVisible(iris);

    m_view->setBackgroundSectionVisible(!irisOrOsiris);
    m_view->setPlotTimeSectionVisible(!irisOrOsiris);
    m_view->setAclimaxSaveVisible(!irisOrOsiris);
    m_view->setFoldMultipleFramesVisible(!irisOrOsiris);
    m_view->setOutputInCm1Visible(!irisOrOsiris);

    m_idrUI->showAnalyserAndReflectionOptions(!toscaOrTfxa);
    m_view->setSPEVisible(!toscaOrTfxa);
    m_view->setAnalysisSectionVisible(!toscaOrTfxa);
    m_view->setCalibVisible(!toscaOrTfxa);
    m_view->setEfixedVisible(!toscaOrTfxa);
  }
}

bool IETPresenter::validate() {
  IETRunData runData = m_view->getRunData();
  UserInputValidator uiv;

  if (!m_view->isRunFilesValid()) {
    uiv.addErrorMessage("Run file range is invalid.");
  }

  if (runData.getInputData().getUseCalibration()) {
    m_view->validateCalibrationFileType(uiv);
  }

  auto rebinDetails = runData.getRebinData();
  if (rebinDetails.getShouldRebin()) {
    if (rebinDetails.getRebinType() == IETRebinType::SINGLE) {
      double rebinWidth = rebinDetails.getRebinWidth();
      if (rebinWidth < 0) {
        bool response = m_view->showRebinWidthPrompt();
        if (response)
          rebinWidth = std::abs(rebinWidth);

        bool rebinValid = !uiv.checkBins(rebinDetails.getRebinLow(), rebinWidth, rebinDetails.getRebinHigh());
        m_view->setSingleRebin(rebinValid);
      }
    } else {
      m_view->validateRebinString(uiv);
    }
  } else {
    m_view->setSingleRebin(false);
    m_view->setMultipleRebin(false);
  }

  auto instrumentDetails = getInstrumentData();
  std::vector<std::string> errors = m_model->validateRunData(runData, instrumentDetails.getDefaultSpectraMin(),
                                                             instrumentDetails.getDefaultSpectraMax());

  for (auto const &error : errors) {
    if (!error.empty())
      uiv.addErrorMessage(QString::fromStdString(error));
  }

  QString error = uiv.generateErrorMessage();
  if (!error.isEmpty())
    showMessageBox(error);

  return validateInstrumentDetails() && uiv.isAllInputValid();
}

void IETPresenter::notifyRunClicked() { runTab(); }

void IETPresenter::run() {
  InstrumentData instrumentData = getInstrumentData();
  IETRunData runData = m_view->getRunData();

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));

  m_outputGroupName = m_model->runIETAlgorithm(m_batchAlgoRunner, instrumentData, runData);
}

void IETPresenter::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  if (!error) {
    InstrumentData instrumentData = getInstrumentData();
    m_outputWorkspaces = m_model->groupWorkspaces(m_outputGroupName, instrumentData.getInstrument(),
                                                  m_view->getGroupOutputOption(), m_view->getGroupOutputCheckbox());
    m_pythonExportWsName = m_outputWorkspaces[0];

    if (m_outputWorkspaces.size() != 0) {
      setOutputPlotOptionsWorkspaces(m_outputWorkspaces);
      m_view->setOutputWorkspaces(m_outputWorkspaces);
      m_view->setSaveEnabled(true);
    }
  }
}

void IETPresenter::notifyPlotRawClicked() {
  InstrumentData instrumentData = getInstrumentData();
  IETPlotData plotParams = m_view->getPlotData();
  std::vector<std::string> errors = m_model->validatePlotData(plotParams);

  if (errors.empty()) {
    m_view->setPlotTimeIsPlotting(true);

    disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
    connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));

    m_batchAlgoRunner->setQueue(m_model->plotRawAlgorithmQueue(instrumentData, plotParams));
    m_batchAlgoRunner->executeBatchAsync();
  } else {
    m_view->setPlotTimeIsPlotting(false);
    for (auto const &error : errors) {
      if (!error.empty())
        showMessageBox(QString::fromStdString(error));
    }
  }
}

void IETPresenter::plotRawComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(plotRawComplete(bool)));

  if (!error) {
    auto const filename = m_view->getFirstFilename();
    std::filesystem::path fileInfo(filename);
    auto const name = fileInfo.filename().string();
    m_plotter->plotSpectra(name + "_grp", "0", SettingsHelper::externalPlotErrorBars());
  }

  m_view->setPlotTimeIsPlotting(false);
}

void IETPresenter::notifySaveClicked() {
  IETSaveData saveData = m_view->getSaveData();
  for (auto const &workspaceName : m_outputWorkspaces)
    if (doesExistInADS(workspaceName))
      m_model->saveWorkspace(workspaceName, saveData);
}

void IETPresenter::notifySaveCustomGroupingClicked() {
  InstrumentData instrumentData = getInstrumentData();
  std::string customGrouping = m_view->getCustomGrouping();

  if (!customGrouping.empty()) {
    m_model->createGroupingWorkspace(instrumentData.getInstrument(), instrumentData.getAnalyser(), customGrouping,
                                     IETGroupingConstants::GROUPING_WS_NAME);
  } else {
    m_view->displayWarning("The custom grouping is empty.");
  }

  if (doesExistInADS(IETGroupingConstants::GROUPING_WS_NAME)) {
    auto const saveDirectory = Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory");
    m_view->showSaveCustomGroupingDialog(IETGroupingConstants::GROUPING_WS_NAME,
                                         IETGroupingConstants::DEFAULT_GROUPING_FILENAME, saveDirectory);
  }
}

void IETPresenter::notifyRunFinished() {
  if (!m_view->isRunFilesValid()) {
    m_view->updateRunButton(false, "unchanged", "Invalid Run(s)",
                            "Cannot find data files for some of the run numbers entered.");
  } else {
    double detailedBalance = m_model->loadDetailedBalance(m_view->getFirstFilename());
    m_view->setDetailedBalance(detailedBalance);
    m_view->updateRunButton();
  }
  m_view->setRunFilesEnabled(true);
}

void IETPresenter::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("ISISEnergyTransfer");
  auto fbSuffixes = filter ? getCalibrationFBSuffixes(tabName) : getCalibrationExtensions(tabName);
  auto wsSuffixes = filter ? getCalibrationWSSuffixes(tabName) : noSuffixes;

  m_view->setFileExtensionsByName(fbSuffixes, wsSuffixes);
}

void IETPresenter::updateRunButton(bool enabled, std::string const &enableOutputButtons, QString const &message,
                                   QString const &tooltip) {
  m_view->updateRunButton(enabled, enableOutputButtons, message, tooltip);
}

void IETPresenter::notifyNewMessage(QString const &message) { showMessageBox(message); }

} // namespace MantidQt::CustomInterfaces