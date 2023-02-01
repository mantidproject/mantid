// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationIqtTab.h"

#include "IndirectSettingsHelper.h"
#include "MantidAPI/ITableWorkspace.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidGeometry/Instrument.h"
#include "MantidQtWidgets/Common/SignalBlocker.h"
#include "MantidQtWidgets/Plotting/RangeSelector.h"

#include "MantidQtWidgets/Common/UserInputValidator.h"

#include <tuple>

using namespace Mantid::API;
using namespace Mantid::Geometry;
using namespace MantidQt::CustomInterfaces;

namespace {
Mantid::Kernel::Logger g_log("Iqt");
} // namespace

namespace MantidQt::CustomInterfaces::IDA {
InelasticDataManipulationIqtTab::InelasticDataManipulationIqtTab(QWidget *parent)
    : InelasticDataManipulationTab(parent), m_view(std::make_unique<InelasticDataManipulationIqtTabView>(parent)),
      m_model(std::make_unique<InelasticDataManipulationIqtTabModel>()), m_iqtResFileType(), m_selectedSpectrum(0) {
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::SpectraTiled));
}

InelasticDataManipulationIqtTab::~InelasticDataManipulationIqtTab() {}

void InelasticDataManipulationIqtTab::setup() {
  // signals / slots & validators
  connect(m_view.get(), SIGNAL(sampDataReady(const QString &)), this, SLOT(plotInput(const QString &)));
  connect(m_view.get(), SIGNAL(resDataReady(const QString &)), this, SLOT(handleResDataReady(const QString &)));
  connect(m_view.get(), SIGNAL(iterationsChanged(int)), this, SLOT(handleIterationsChanged(int)));
  connect(m_view.get(), SIGNAL(errorsClicked(int)), this, SLOT(handleErrorsClicked(int)));
  connect(m_view.get(), SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(handleValueChanged(QtProperty *, double)));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  connect(m_view.get(), SIGNAL(showMessageBox(const QString &)), this, SIGNAL(showMessageBox(const QString &)));
  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));
  connect(m_view.get(), SIGNAL(plotCurrentPreview()), this, SLOT(plotCurrentPreview()));

  connect(m_view.get(), SIGNAL(previewSpectrumChanged(int)), this, SLOT(handlePreviewSpectrumChanged(int)));

  m_view->setup();
}

void InelasticDataManipulationIqtTab::run() {
  m_view->setWatchADS(false);
  setRunIsRunning(true);

  m_view->updateDisplayedBinParameters();

  // Construct the result workspace for Python script export
  QString const sampleName = QString::fromStdString(m_view->getSampleName());
  m_pythonExportWsName = sampleName.left(sampleName.lastIndexOf("_")).toStdString() + "_iqt";
  m_model->setupTransformToIqt(m_batchAlgoRunner, m_pythonExportWsName);
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle algorithm completion.
 *
 * @param error If the algorithm failed
 */
void InelasticDataManipulationIqtTab::algorithmComplete(bool error) {
  m_view->setWatchADS(true);
  setRunIsRunning(false);
  if (error)
    m_view->setSaveResultEnabled(false);
  else
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
}

/**
 * Handle saving of workspace
 */
void InelasticDataManipulationIqtTab::saveClicked() {
  checkADSForPlotSaveWorkspace(m_pythonExportWsName, false);
  addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatchAsync();
}

void InelasticDataManipulationIqtTab::runClicked() {
  clearOutputPlotOptionsWorkspaces();
  runTab();
}

/**
 * Ensure we have present and valid file/ws inputs.
 *
 * The underlying Fourier transform of Iqt
 * also means we must enforce several rules on the parameters.
 */
bool InelasticDataManipulationIqtTab::validate() { return m_view->validate(); }

void InelasticDataManipulationIqtTab::handleResDataReady(const QString &resWorkspace) {
  m_model->setResWorkspace(resWorkspace.toStdString());
}

void InelasticDataManipulationIqtTab::handleIterationsChanged(int iterations) {
  m_model->setNIterations(std::to_string(iterations));
}

void InelasticDataManipulationIqtTab::handleValueChanged(QtProperty *prop, double value) {
  if (prop->propertyName() == "ELow") {
    m_model->setEnergyMin(value);
  } else if (prop->propertyName() == "EHigh") {
    m_model->setEnergyMax(value);
  } else if (prop->propertyName() == "SampleBinning") {
    m_model->setNumBins(value);
  }
}

void InelasticDataManipulationIqtTab::handleErrorsClicked(int state) { m_model->setCalculateErrors(state); }

void InelasticDataManipulationIqtTab::handlePreviewSpectrumChanged(int spectra) {
  setSelectedSpectrum(spectra);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
}

void InelasticDataManipulationIqtTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Iqt");
  m_view->setSampleFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setSampleWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
  m_view->setResolutionFBSuffixes(filter ? getResolutionFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setResolutionWSSuffixes(filter ? getResolutionWSSuffixes(tabName) : noSuffixes);
}

void InelasticDataManipulationIqtTab::plotInput(const QString &wsname) {
  MatrixWorkspace_sptr workspace;
  try {
    workspace = AnalysisDataService::Instance().retrieveWS<MatrixWorkspace>(wsname.toStdString());
    setInputWorkspace(workspace);
  } catch (Mantid::Kernel::Exception::NotFoundError &) {
    showMessageBox(QString("Unable to retrieve workspace: " + wsname));
    m_view->setPreviewSpectrumMaximum(0);
    return;
  }

  m_view->setPreviewSpectrumMaximum(static_cast<int>(getInputWorkspace()->getNumberHistograms()) - 1);
  m_view->plotInput(getInputWorkspace(), getSelectedSpectrum());
  m_view->setRangeSelectorDefault(getInputWorkspace(), getXRangeFromWorkspace(getInputWorkspace()));
  m_view->updateDisplayedBinParameters();
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
 * Plots the current preview workspace, if none is set, plots
 * the selected spectrum of the current input workspace.
 */
void InelasticDataManipulationIqtTab::plotCurrentPreview() {
  auto previewWs = getPreviewPlotWorkspace();
  auto inputWs = getInputWorkspace();
  auto index = boost::numeric_cast<size_t>(m_selectedSpectrum);
  auto const errorBars = IndirectSettingsHelper::externalPlotErrorBars();

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
    showMessageBox("Workspace not found - data may not be loaded.");
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

} // namespace MantidQt::CustomInterfaces::IDA
