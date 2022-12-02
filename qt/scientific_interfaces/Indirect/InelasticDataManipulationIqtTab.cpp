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

/**
 * Calculate the number of bins in the sample & resolution workspaces
 * @param wsName The sample workspace name
 * @param resName the resolution woskapce name
 * @param energyMin Minimum energy for chosen bin range
 * @param energyMax Maximum energy for chosen bin range
 * @param binReductionFactor The factor by which to reduce the number of bins
 * @return A 4-tuple where the first entry denotes whether the
 * calculation was successful or not. The final 3 values
 * are EWidth, SampleBins, ResolutionBins if the calculation succeeded,
 * otherwise they are undefined.
 */
std::tuple<bool, float, int, int> calculateBinParameters(std::string const &wsName, std::string const &resName,
                                                         double energyMin, double energyMax,
                                                         double binReductionFactor) {
  ITableWorkspace_sptr propsTable;
  try {
    const auto paramTableName = "__IqtProperties_temp";
    auto toIqt = AlgorithmManager::Instance().createUnmanaged("TransformToIqt");
    toIqt->initialize();
    toIqt->setChild(true); // record this as internal
    toIqt->setProperty("SampleWorkspace", wsName);
    toIqt->setProperty("ResolutionWorkspace", resName);
    toIqt->setProperty("ParameterWorkspace", paramTableName);
    toIqt->setProperty("EnergyMin", energyMin);
    toIqt->setProperty("EnergyMax", energyMax);
    toIqt->setProperty("BinReductionFactor", binReductionFactor);
    toIqt->setProperty("DryRun", true);
    toIqt->execute();
    propsTable = toIqt->getProperty("ParameterWorkspace");
    // the algorithm can create output even if it failed...
    auto deleter = AlgorithmManager::Instance().create("DeleteWorkspace");
    deleter->initialize();
    deleter->setChild(true);
    deleter->setProperty("Workspace", paramTableName);
    deleter->execute();
  } catch (std::exception &) {
    return std::make_tuple(false, 0.0f, 0, 0);
  }
  assert(propsTable);
  return std::make_tuple(true, propsTable->getColumn("EnergyWidth")->cell<float>(0),
                         propsTable->getColumn("SampleOutputBins")->cell<int>(0),
                         propsTable->getColumn("ResolutionBins")->cell<int>(0));
}
} // namespace

namespace MantidQt::CustomInterfaces::IDA {
InelasticDataManipulationIqtTab::InelasticDataManipulationIqtTab(QWidget *parent)
    : InelasticDataManipulationTab(parent), m_view(std::make_unique<InelasticDataManipulationIqtTabView>(parent)),
      m_iqtResFileType(), m_selectedSpectrum(0) {
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::SpectraTiled));
}

InelasticDataManipulationIqtTab::~InelasticDataManipulationIqtTab() {}

void InelasticDataManipulationIqtTab::setup() {
  // signals / slots & validators
  connect(m_view.get(), SIGNAL(sampDataReady(const QString &)), this, SLOT(plotInput(const QString &)));

  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  connect(m_view.get(), SIGNAL(showMessageBox(const QString &)), this, SIGNAL(showMessageBox(const QString &)));
  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));
  connect(m_view.get(), SIGNAL(plotCurrentPreview()), this, SLOT(plotCurrentPreview()));

  connect(m_view.get(), SIGNAL(PreviewSpectrumChanged(int)), this, SLOT(handlePreviewSpectrumChanged(int)));
}

void InelasticDataManipulationIqtTab::run() {
  m_view->setWatchADS(false);
  setRunIsRunning(true);

  m_view->updateDisplayedBinParameters();

  // Construct the result workspace for Python script export
  QString const sampleName = QString::fromStdString(m_view->getSampleName());
  m_pythonExportWsName = sampleName.left(sampleName.lastIndexOf("_")).toStdString() + "_iqt";

  auto const wsName = m_view->getSampleName();
  auto const resName = m_view->getResolutionName();
  auto const nIterations = m_view->getIterations();
  bool const calculateErrors = m_view->getCalculateErrors();

  double const energyMin = m_view->getELow();
  double const energyMax = m_view->getEHigh();
  double const numBins = m_view->getSampleBinning();

  auto IqtAlg = AlgorithmManager::Instance().create("TransformToIqt");
  IqtAlg->initialize();

  IqtAlg->setProperty("SampleWorkspace", wsName);
  IqtAlg->setProperty("ResolutionWorkspace", resName);
  IqtAlg->setProperty("NumberOfIterations", nIterations);
  IqtAlg->setProperty("CalculateErrors", calculateErrors);

  IqtAlg->setProperty("EnergyMin", energyMin);
  IqtAlg->setProperty("EnergyMax", energyMax);
  IqtAlg->setProperty("BinReductionFactor", numBins);
  IqtAlg->setProperty("OutputWorkspace", m_pythonExportWsName);

  IqtAlg->setProperty("DryRun", false);

  m_batchAlgoRunner->addAlgorithm(IqtAlg);
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
