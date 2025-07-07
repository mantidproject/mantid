// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ISISEnergyTransferPresenter.h"
#include "ISISEnergyTransferData.h"
#include "ISISEnergyTransferModel.h"
#include "ISISEnergyTransferView.h"
#include "MantidQtWidgets/Common/WorkspaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/InterfaceUtils.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/WorkspaceGroup.h"
#include "MantidKernel/ConfigService.h"

#include "MantidQtWidgets/Common/QtJobRunner.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include <MantidQtWidgets/Common/ParseKeyValueString.h>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <filesystem>
#include <regex>

using namespace Mantid::API;
using namespace MantidQt::MantidWidgets::WorkspaceUtils;
using namespace MantidQt::CustomInterfaces::InterfaceUtils;

namespace {
constexpr auto REDUCTION_ALG_NAME = "ISISIndirectEnergyTransfer";
constexpr auto PLOT_PREPROCESS_ALG_NAME = "GroupDetectors";
const std::vector<std::string> SUFFIXES = {"_Reduced"};

enum class AlgorithmType { REDUCTION, PLOT_RAW_PREPROCESS };

AlgorithmType algorithmType(MantidQt::API::IConfiguredAlgorithm_sptr &configuredAlg) {
  auto const &name = configuredAlg->algorithm()->name();
  if (name == REDUCTION_ALG_NAME) {
    return AlgorithmType::REDUCTION;
  } else if (name == PLOT_PREPROCESS_ALG_NAME) {
    return AlgorithmType::PLOT_RAW_PREPROCESS;
  } else {
    throw std::logic_error("ISIS Energy Transfer tab error: callback from invalid algorithm " + name);
  }
}

} // namespace

namespace MantidQt::CustomInterfaces {

IETPresenter::IETPresenter(IDataReduction *idrUI, IIETView *view, std::unique_ptr<IIETModel> model,
                           std::unique_ptr<API::IAlgorithmRunner> algorithmRunner)
    : DataReductionTab(idrUI, std::move(algorithmRunner)), m_view(view), m_model(std::move(model)) {
  m_view->subscribePresenter(this);
  m_algorithmRunner->subscribe(this);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
  setOutputPlotOptionsPresenter(m_view->getPlotOptionsView(), PlotWidget::SpectraSliceSurface);
  setOutputNamePresenter(m_view->getOutputName());
  m_outputNamePresenter->setWsSuffixes(SUFFIXES);
  m_outputNamePresenter->hideOutputNameBox();
}

void IETPresenter::validateInstrumentDetails(IUserInputValidator *validator) const {
  auto const instrument = getInstrumentName().toStdString();
  if (instrument.empty()) {
    validator->addErrorMessage("Please select a valid facility and/or instrument.");
  }

  QMap<QString, QString> instrumentDetails = getInstrumentDetails();
  std::set<std::string> keys = {"spectra-min", "spectra-max"};
  for (const auto &key : keys) {
    if (!instrumentDetails.contains(QString::fromStdString(key)) ||
        instrumentDetails[QString::fromStdString(key)].isEmpty()) {
      validator->addErrorMessage("Could not find " + key + " for the " + instrument +
                                 " instrument. Please select a valid instrument.");
      break;
    }
  }
}

InstrumentData IETPresenter::getInstrumentData() const {
  QMap<QString, QString> instrumentDetails = getInstrumentDetails();

  return InstrumentData(
      getInstrumentName().toStdString(), getAnalyserName().toStdString(), getReflectionName().toStdString(),
      instrumentDetails["spectra-min"].toInt(), instrumentDetails["spectra-max"].toInt(),
      instrumentDetails["Efixed"].toDouble(), instrumentDetails["rebin-default"].toStdString(),
      instrumentDetails["cm-1-convert-choice"] == "true", instrumentDetails["save-nexus-choice"] == "true",
      instrumentDetails["save-ascii-choice"] == "true", instrumentDetails["fold-frames-choice"] == "true");
}

void IETPresenter::updateInstrumentConfiguration() {
  auto validator = std::make_unique<UserInputValidator>();
  validateInstrumentDetails(validator.get());
  const auto error = validator->generateErrorMessage();
  if (!error.empty()) {
    m_view->displayWarning(error);
    return;
  }

  InstrumentData instrumentDetails = getInstrumentData();
  auto const instrumentName = instrumentDetails.getInstrument();

  // spectraRange & Efixed
  auto const specMin = instrumentDetails.getDefaultSpectraMin();
  auto const specMax = instrumentDetails.getDefaultSpectraMax();
  m_view->setInstrumentSpectraRange(specMin, specMax);
  m_view->setInstrumentEFixed(instrumentName, instrumentDetails.getDefaultEfixed());

  // Rebinning
  auto const rebinDefault = instrumentDetails.getDefaultRebin();
  std::vector<double> rebinParams;
  if (!rebinDefault.empty()) {
    std::vector<std::string> rebinParamsStr;
    boost::split(rebinParamsStr, rebinDefault, boost::is_any_of(","));
    std::for_each(rebinParamsStr.begin(), rebinParamsStr.end(),
                  [&rebinParams](auto &param) { rebinParams.push_back(std::stod(param)); });
  } else
    rebinParams = {0, 0, 0};

  int rebinTab = (int)(rebinParams.size() != 3);
  std::string rebinString = !rebinDefault.empty() ? rebinDefault : "";
  m_view->setInstrumentRebinning(rebinParams, rebinString, rebinDefault.empty(), rebinTab);

  // Grouping
  m_view->setInstrumentGrouping(instrumentName);

  // Instrument spec defaults
  bool irsORosiris = std::regex_search(instrumentName, std::regex("(^OSIRIS$)|(^IRIS$)"));
  bool toscaORtfxa = std::regex_search(instrumentName, std::regex("(^TOSCA$)|(^TFXA$)"));
  m_idrUI->showAnalyserAndReflectionOptions(!toscaORtfxa);
  std::map<std::string, bool> specMap{{"irsORosiris", !irsORosiris},
                                      {"toscaORtfxa", !toscaORtfxa},
                                      {"defaultEUnits", instrumentDetails.getDefaultUseDeltaEInWavenumber()},
                                      {"defaultSaveNexus", instrumentDetails.getDefaultSaveNexus()},
                                      {"defaultSaveASCII", instrumentDetails.getDefaultSaveASCII()},
                                      {"defaultFoldMultiple", instrumentDetails.getDefaultFoldMultipleFrames()}};
  m_view->setInstrumentSpecDefault(specMap);
}

void IETPresenter::handleRun() {
  InstrumentData instrumentData = getInstrumentData();
  IETRunData runData = m_view->getRunData();

  std::string outputLabel = m_outputNamePresenter->getCurrentLabel();
  std::string outputGroupName = m_outputNamePresenter->generateOutputLabel();

  m_view->setEnableOutputOptions(false);

  m_algorithmRunner->execute(m_model->energyTransferAlgorithm(instrumentData, runData, outputGroupName, outputLabel));
}

void IETPresenter::handleValidation(IUserInputValidator *validator) const {
  IETRunData runData = m_view->getRunData();

  if (!m_view->isRunFilesValid()) {
    validator->addErrorMessage("Run file range is invalid.");
  }

  if (runData.getInputData().getUseCalibration()) {
    m_view->validateCalibrationFileType(validator);
  }

  auto rebinDetails = runData.getRebinData();
  if (rebinDetails.getShouldRebin()) {
    if (rebinDetails.getRebinType() == IETRebinType::SINGLE) {
      double rebinWidth = rebinDetails.getRebinWidth();
      if (rebinWidth < 0) {
        bool response = m_view->showRebinWidthPrompt();
        if (response)
          rebinWidth = std::abs(rebinWidth);

        bool rebinValid = !validator->checkBins(rebinDetails.getRebinLow(), rebinWidth, rebinDetails.getRebinHigh());
        m_view->setSingleRebin(rebinValid);
      }
    } else {
      m_view->validateRebinString(validator);
    }
  } else {
    m_view->setSingleRebin(false);
    m_view->setMultipleRebin(false);
  }

  auto instrumentDetails = getInstrumentData();
  std::vector<std::string> errors = m_model->validateRunData(runData);
  auto const groupingError = m_view->validateGroupingProperties(instrumentDetails.getDefaultSpectraMin(),
                                                                instrumentDetails.getDefaultSpectraMax());
  if (groupingError)
    errors.emplace_back(*groupingError);

  for (auto const &error : errors) {
    if (!error.empty())
      validator->addErrorMessage(error);
  }

  validateInstrumentDetails(validator);
}

void IETPresenter::notifyFindingRun() { m_runPresenter->setRunText("Finding files..."); }

void IETPresenter::notifyBatchComplete(API::IConfiguredAlgorithm_sptr &lastAlgorithm, bool error) {
  m_runPresenter->setRunEnabled(true);
  if (!lastAlgorithm || error) {
    m_view->setEnableOutputOptions(false);
    m_view->setPlotTimeIsPlotting(false);
    return;
  }
  switch (algorithmType(lastAlgorithm)) {
  case AlgorithmType::REDUCTION:
    handleReductionComplete();
    return;
  case AlgorithmType::PLOT_RAW_PREPROCESS:
    handlePlotRawPreProcessComplete();
    return;
  default:
    throw std::logic_error("Unexpected ISIS Energy Transfer tab error: callback from invalid algorithm batch.");
  }
}

void IETPresenter::handleReductionComplete() {
  m_runPresenter->setRunEnabled(true);
  m_view->setEnableOutputOptions(true);

  InstrumentData instrumentData = getInstrumentData();
  auto const outputWorkspaceNames =
      m_model->groupWorkspaces(m_model->outputGroupName(), instrumentData.getInstrument(),
                               m_view->getGroupOutputOption(), m_view->getGroupOutputCheckbox());
  if (!outputWorkspaceNames.empty())
    m_pythonExportWsName = outputWorkspaceNames[0];

  setOutputPlotOptionsWorkspaces(outputWorkspaceNames);
  m_view->setSaveEnabled(!outputWorkspaceNames.empty());
}

void IETPresenter::handlePlotRawPreProcessComplete() {
  m_view->setPlotTimeIsPlotting(false);
  auto const filename = m_view->getFirstFilename();
  std::filesystem::path fileInfo(filename);
  auto const name = fileInfo.filename().string();
  m_plotter->plotSpectra(name + "_grp", "0", SettingsHelper::externalPlotErrorBars());
}

void IETPresenter::notifyPlotRawClicked() {
  InstrumentData instrumentData = getInstrumentData();
  IETPlotData plotParams = m_view->getPlotData();
  std::vector<std::string> errors = m_model->validatePlotData(plotParams);

  if (errors.empty()) {
    m_view->setPlotTimeIsPlotting(true);
    m_algorithmRunner->execute(m_model->plotRawAlgorithmQueue(instrumentData, plotParams));
  } else {
    m_view->setPlotTimeIsPlotting(false);
    for (auto const &error : errors) {
      if (!error.empty())
        m_view->showMessageBox(error);
    }
  }
}

void IETPresenter::notifySaveClicked() {
  IETSaveData saveData = m_view->getSaveData();
  for (auto const &workspaceName : m_model->outputWorkspaceNames())
    if (doesExistInADS(workspaceName))
      m_model->saveWorkspace(workspaceName, saveData);
}

void IETPresenter::notifySaveCustomGroupingClicked(std::string const &customGrouping) {
  InstrumentData instrumentData = getInstrumentData();

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
    m_runPresenter->setRunText("Invalid Run(s)");
  } else {
    double detailedBalance = m_model->loadDetailedBalance(m_view->getFirstFilename());
    m_view->setDetailedBalance(detailedBalance);
    m_runPresenter->setRunEnabled(true);
    std::string output = m_model->getOutputGroupName(getInstrumentData(), m_view->getInputText());
    m_outputNamePresenter->setOutputWsBasename(output);
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

void IETPresenter::setLoadHistory(bool doLoadHistory) { m_view->setLoadHistory(doLoadHistory); }

} // namespace MantidQt::CustomInterfaces
