// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationIqtTab.h"

#include "Common/InterfaceUtils.h"
#include "Common/SettingsHelper.h"
#include "Common/WorkspaceUtils.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <tuple>

using namespace Mantid::API;
using namespace Mantid::Geometry;

namespace {
Mantid::Kernel::Logger g_log("Iqt");
} // namespace

namespace MantidQt {
namespace CustomInterfaces {

InelasticDataManipulationIqtTab::InelasticDataManipulationIqtTab(QWidget *parent, IIqtView *view)
    : InelasticDataManipulationTab(parent), m_view(view),
      m_model(std::make_unique<InelasticDataManipulationIqtTabModel>()), m_selectedSpectrum(0) {
  m_view->subscribePresenter(this);
  setOutputPlotOptionsPresenter(
      std::make_unique<OutputPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::SpectraTiled));
}

void InelasticDataManipulationIqtTab::setup() { m_view->setup(); }

void InelasticDataManipulationIqtTab::handleSampDataReady(const std::string &wsname) {
  MatrixWorkspace_sptr workspace;
  try {
    workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname);
    setInputWorkspace(workspace);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    m_view->showMessageBox("Unable to retrieve workspace: " + wsname);
    m_view->setPreviewSpectrumMaximum(0);
    return;
  }

  m_view->setPreviewSpectrumMaximum(static_cast<int>(getInputWorkspace()->getNumberHistograms()) - 1);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  m_view->setRangeSelectorDefault(getInputWorkspace(), WorkspaceUtils::getXRangeFromWorkspace(getInputWorkspace()));
  m_view->updateDisplayedBinParameters();
}

void InelasticDataManipulationIqtTab::handleResDataReady(const std::string &resWorkspace) {
  m_view->updateDisplayedBinParameters();
  m_model->setResWorkspace(resWorkspace);
}

void InelasticDataManipulationIqtTab::handleIterationsChanged(int iterations) {
  m_model->setNIterations(std::to_string(iterations));
}

void InelasticDataManipulationIqtTab::handleRunClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}
/**
 * Handle saving of workspace
 */
void InelasticDataManipulationIqtTab::handleSaveClicked() {
  checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);
  addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void InelasticDataManipulationIqtTab::handlePlotCurrentPreview() {
  auto previewWs = getPreviewPlotWorkspace();
  auto inputWs = getInputWorkspace();
  auto index = boost::numeric_cast<size_t>(m_selectedSpectrum);
  auto const errorBars = SettingsHelper::externalPlotErrorBars();

  // Check a workspace has been selected
  if (previewWs) {

    if (inputWs && previewWs->getName() == inputWs->getName()) {
      m_plotter->plotSpectra(previewWs->getName(), std::to_string(index), errorBars);
    } else {
      m_plotter->plotSpectra(previewWs->getName(), "0-2", errorBars);
    }
  } else if (inputWs && index < inputWs->getNumberHistograms()) {
    m_plotter->plotSpectra(inputWs->getName(), std::to_string(index), errorBars);
  } else
    m_view->showMessageBox("Workspace not found - data may not be loaded.");
}

void InelasticDataManipulationIqtTab::handleErrorsClicked(int state) { m_model->setCalculateErrors(state); }

void InelasticDataManipulationIqtTab::handleNormalizationClicked(int state) { m_model->setEnforceNormalization(state); }

void InelasticDataManipulationIqtTab::handleValueChanged(std::string const &propName, double value) {
  if (propName == "ELow") {
    m_model->setEnergyMin(value);
  } else if (propName == "EHigh") {
    m_model->setEnergyMax(value);
  } else if (propName == "SampleBinning") {
    m_model->setNumBins(value);
  }
}

void InelasticDataManipulationIqtTab::handlePreviewSpectrumChanged(int spectra) {
  setSelectedSpectrum(spectra);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

void InelasticDataManipulationIqtTab::run() {
  m_view->setWatchADS(false);
  setRunIsRunning(true);

  m_view->updateDisplayedBinParameters();

  // Construct the result workspace for Python script export
  std::string sampleName = m_view->getSampleName();
  m_pythonExportWsName = sampleName.replace(sampleName.find_last_of("_"), sampleName.size(), "_iqt");
  m_model->setupTransformToIqt(m_batchAlgoRunner, m_pythonExportWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle algorithm completion.
 *
 * @param error If the algorithm failed
 */
void InelasticDataManipulationIqtTab::runComplete(bool error) {
  m_view->setWatchADS(true);
  setRunIsRunning(false);
  if (error)
    m_view->setSaveResultEnabled(false);
  else
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
}

/**
 * Ensure we have present and valid file/ws inputs.
 *
 * The underlying Fourier transform of Iqt
 * also means we must enforce several rules on the parameters.
 */
bool InelasticDataManipulationIqtTab::validate() { return m_view->validate(); }

void InelasticDataManipulationIqtTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Iqt");
  m_view->setSampleFBSuffixes(filter ? InterfaceUtils::getSampleFBSuffixes(tabName)
                                     : InterfaceUtils::getExtensions(tabName));
  m_view->setSampleWSSuffixes(filter ? InterfaceUtils::getSampleWSSuffixes(tabName) : noSuffixes);
  m_view->setResolutionFBSuffixes(filter ? InterfaceUtils::getResolutionFBSuffixes(tabName)
                                         : InterfaceUtils::getExtensions(tabName));
  m_view->setResolutionWSSuffixes(filter ? InterfaceUtils::getResolutionWSSuffixes(tabName) : noSuffixes);
}

void InelasticDataManipulationIqtTab::setButtonsEnabled(bool enabled) {
  m_view->setRunEnabled(enabled);
  m_view->setSaveResultEnabled(enabled);
}

void InelasticDataManipulationIqtTab::setRunIsRunning(bool running) {
  m_view->setRunText(running);
  setButtonsEnabled(!running);
}

/**
 * Retrieves the selected spectrum.
 *
 * @return  The selected spectrum.
 */
int InelasticDataManipulationIqtTab::getSelectedSpectrum() const { return m_selectedSpectrum; }

/**
 * Sets the selected spectrum.
 *
 * @param spectrum  The spectrum to set.
 */
void InelasticDataManipulationIqtTab::setSelectedSpectrum(int spectrum) { m_selectedSpectrum = spectrum; }

/**
 * Retrieves the input workspace to be used in data analysis.
 *
 * @return  The input workspace to be used in data analysis.
 */
MatrixWorkspace_sptr InelasticDataManipulationIqtTab::getInputWorkspace() const { return m_inputWorkspace; }

/**
 * Sets the input workspace to be used in data analysis.
 *
 * @param inputWorkspace  The workspace to set.
 */
void InelasticDataManipulationIqtTab::setInputWorkspace(MatrixWorkspace_sptr inputWorkspace) {
  m_model->setSampleWorkspace(inputWorkspace->getName());
  m_inputWorkspace = std::move(inputWorkspace);
}

/**
 * Retrieves the workspace containing the data to be displayed in
 * the preview plot.
 *
 * @return  The workspace containing the data to be displayed in
 *          the preview plot.
 */
MatrixWorkspace_sptr InelasticDataManipulationIqtTab::getPreviewPlotWorkspace() {
  return m_previewPlotWorkspace.lock();
}

/**
 * Sets the workspace containing the data to be displayed in the
 * preview plot.
 *
 * @param previewPlotWorkspace The workspace to set.
 */
void InelasticDataManipulationIqtTab::setPreviewPlotWorkspace(const MatrixWorkspace_sptr &previewPlotWorkspace) {
  m_previewPlotWorkspace = previewPlotWorkspace;
}
} // namespace CustomInterfaces
} // namespace MantidQt
