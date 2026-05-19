// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "ContainerSubtractionPresenter.h"

#include <MantidQtWidgets/Common/ParseKeyValueString.h>
#include <MantidQtWidgets/Spectroscopy/InterfaceUtils.h>
#include <MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h>

#include <utility>

namespace {
Mantid::Kernel::Logger g_log("ContainerSubtraction");
}

using namespace Mantid::API;
namespace MantidQt::CustomInterfaces {
ContainerSubtractionPresenter::ContainerSubtractionPresenter(QWidget *parent,
                                                             std::unique_ptr<API::IAlgorithmRunner> algoRunner,
                                                             std::unique_ptr<IContainerSubtractionModel> model,
                                                             IContainerSubtractionView *view)
    : CorrectionsTab(parent, std::move(algoRunner)), m_model(std::move(model)), m_view(view) {
  m_view->subscribe(this);
  setRunWidgetPresenter(std::make_unique<RunPresenter>(this, m_view->getRunView()));
  setOutputPlotOptionsPresenter(m_view->getPlotOptions(), PlotWidget::SpectraSliceSurface);
  setOutputNamePresenter(m_view->getOutputNameView());
  m_outputNamePresenter->setWsSuffixes(
      MantidWidgets::qStringListToStdVector(InterfaceUtils::getSampleWSSuffixes("ContainerSubtraction")));
}

void ContainerSubtractionPresenter::handleValidation(IUserInputValidator *validator) const {
  // Check valid inputs
  m_view->validate(validator);
  if (auto sampleWS = m_model->sampleWS(), canWS = m_model->canWS(); sampleWS && canWS) {
    // Check Sample is of same type as container
    const auto &containerType = canWS->YUnit();
    const auto &sampleType = sampleWS->YUnit();

    g_log.debug() << "Sample Y-Unit is: " << sampleType << '\n';
    g_log.debug() << "Container Y-Unit is: " << containerType << '\n';

    if (containerType != sampleType)
      validator->addErrorMessage("Sample and can workspaces must contain the same "
                                 "type of data; have the same Y-Unit.");

    // Check sample has the same number of Histograms as the container
    const size_t sampleHist = sampleWS->getNumberHistograms();
    const size_t containerHist = canWS->getNumberHistograms();

    if (sampleHist != containerHist) {
      validator->addErrorMessage(" Sample and Container do not have a matching number of Histograms.");
    }
  } else {
    validator->addErrorMessage("Sample or Container workspaces are not loaded.");
  }
}

void ContainerSubtractionPresenter::handleRun() {
  clearOutputPlotOptionsWorkspaces();
  m_view->enableSaveButton(false);
  auto doRebin(false);
  if ((m_view->getShift() == 0.0) && !checkWorkspaceBinningMatches(m_model->sampleWS(), m_model->canWS())) {
    doRebin = requestRebinToSample();
    if (!doRebin) {
      g_log.error("Cannot apply container corrections using a sample and "
                  "container with different binning.");
      m_runPresenter->setRunEnabled(true);
      return;
    }
  }

  auto confAlg = m_model->prepareSubtraction(m_view->getShift(), m_view->getScale(), doRebin);
  m_algorithmRunner->execute(std::move(confAlg));
}

void ContainerSubtractionPresenter::runComplete(const Mantid::API::IAlgorithm_sptr algorithm, const bool error) {
  if (error) {
    showMessageBox("Error on subtraction algorithm");
    return;
  }
  m_pythonExportWsName = m_outputNamePresenter->generateOutputLabel();
  const MatrixWorkspace_sptr outWS = algorithm->getProperty("OutputWorkspace");
  AnalysisDataService::Instance().addOrReplace(m_pythonExportWsName, outWS);
  m_model->setSubtractedWS(m_pythonExportWsName);
  m_outputNamePresenter->generateWarningLabel();
  updatePlot(m_view->getSpNo());

  if (const auto shiftX = m_view->getShift(); shiftX != 0.0) {
    m_model->addShiftLog(shiftX);
  }

  m_view->enableSaveButton(true);
  setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
}

std::string ContainerSubtractionPresenter::createOutputName() {
  const auto &sampleName = m_model->sampleWS()->getName();
  std::string sampleNameSuffix;
  for (const auto &qSuffix : InterfaceUtils::getSampleWSSuffixes("ContainerSubtraction")) {
    auto suffix = qSuffix.toStdString();
    if (sampleName.ends_with(suffix)) {
      sampleNameSuffix = std::move(suffix);
      break;
    }
  }
  const auto containerName = prepareContainerName(m_model->canWS()->getName());
  return sampleName.substr(0, sampleName.find_last_of("_")) + "_Subtract_" + containerName + sampleNameSuffix;
}

void ContainerSubtractionPresenter::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("ContainerSubtraction");
  m_view->setSampleFBSuffixes(filter ? InterfaceUtils::getSampleFBSuffixes(tabName)
                                     : InterfaceUtils::getExtensions(tabName));
  m_view->setSampleWSSuffixes(filter ? InterfaceUtils::getSampleWSSuffixes(tabName) : noSuffixes);
  m_view->setCanFBSuffixes(filter ? InterfaceUtils::getContainerFBSuffixes(tabName)
                                  : InterfaceUtils::getExtensions(tabName));
  m_view->setCanWSSuffixes(filter ? InterfaceUtils::getContainerWSSuffixes(tabName) : noSuffixes);
}

bool ContainerSubtractionPresenter::requestRebinToSample() const { return m_view->requestRebinToSample(); }

void ContainerSubtractionPresenter::setLoadHistory(bool doLoadHistory) { m_view->setLoadHistory(doLoadHistory); }

void ContainerSubtractionPresenter::updateOutputName() {
  if (m_model->sampleWS() && m_model->canWS()) {
    m_outputNamePresenter->setOutputWsBasename(createOutputName());
  }
}

void ContainerSubtractionPresenter::handleCanReady(const std::string &dataName) {
  m_model->setCanWS(dataName);
  updateNewDataEntry(m_model->canWS());
  updateOutputName();
}

void ContainerSubtractionPresenter::handleSampleReady(const std::string &dataName) {
  m_model->setSampleWS(dataName);
  updateNewDataEntry(m_model->sampleWS());
  updateOutputName();
}

void ContainerSubtractionPresenter::updateNewDataEntry(const MatrixWorkspace_sptr &ws) {
  if (ws) {
    m_model->removeSubtractedWS();
    const auto maxWsIndex = static_cast<int>(ws->getNumberHistograms() - 1);
    const auto max = m_view->getSpMax() != 0 ? std::min(maxWsIndex, m_view->getSpMax()) : maxWsIndex;
    m_view->setSpMax(max);
    updatePlot(0);
  }
}

void ContainerSubtractionPresenter::handleUpdateContainerPlot() {
  const auto shift = m_view->getShift();
  const auto scale = m_view->getScale();
  m_model->updateContainer(shift, scale);
  updatePlot(m_view->getSpNo());
}

void ContainerSubtractionPresenter::loadSettings(const QSettings &settings) { m_view->loadSettings(settings); }

void ContainerSubtractionPresenter::handleSaveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void ContainerSubtractionPresenter::updatePlot(int specNo) {
  m_view->clearPlot();
  if (const auto sampleWS = m_model->sampleWS(); sampleWS) {
    m_view->plotSpectrum(CSCurves::SAMPLE, sampleWS, specNo);
  }
  if (const auto canWS = m_model->canWS(), modCanWS = m_model->modCanWS(); canWS || modCanWS) {
    m_view->plotSpectrum(CSCurves::CONTAINER, modCanWS ? modCanWS : canWS, specNo);
  }
  if (const auto subtractedWS = m_model->subtractedWS(); subtractedWS) {
    m_view->plotSpectrum(CSCurves::SUBTRACTED, subtractedWS, specNo);
  }
}

void ContainerSubtractionPresenter::handlePlotPreviewClicked() {
  const auto workspaceNames = m_model->getAllValidWorkspaceNames();
  const auto index = m_view->getSpNo();
  const auto indices = std::vector(workspaceNames.size(), index);

  m_plotter->plotCorrespondingSpectra(workspaceNames, indices,
                                      std::vector(workspaceNames.size(), SettingsHelper::externalPlotErrorBars()));
}

} // namespace MantidQt::CustomInterfaces
