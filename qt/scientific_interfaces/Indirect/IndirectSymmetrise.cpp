// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSymmetrise.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/SingleSelector.h"

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("IndirectSymmetrise");
} // namespace

namespace MantidQt {
using MantidWidgets::AxisID;
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
IndirectSymmetrise::IndirectSymmetrise(IndirectDataReduction *idrUI, QWidget *parent)
    : IndirectDataReductionTab(idrUI, parent), m_adsInstance(Mantid::API::AnalysisDataService::Instance()),
      m_view(std::make_unique<IndirectSymmetriseView>(parent)) {
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra));

  // SIGNAL/SLOT CONNECTIONS
  // Plot miniplot when file has finished loading
  connect(m_view.get(), SIGNAL(dataReady(QString const &)), this, SLOT(handleDataReady(QString const &)));
  // Preview symmetrise
  connect(m_view.get(), SIGNAL(previewClicked()), this, SLOT(preview()));
  // Handle running, plotting and saving
  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));
}

IndirectSymmetrise::~IndirectSymmetrise() { m_propTrees["SymmPropTree"]->unsetFactoryForManager(m_dblManager); }

void IndirectSymmetrise::setup() {}

/**
 * Handles the event of data being loaded. Validates the loaded data.
 *
 * @param dataName The name of the data that has been loaded
 */
void IndirectSymmetrise::handleDataReady(QString const &dataName) {
  if (m_view->validate()) {
    plotNewData(dataName);
  }
}

bool IndirectSymmetrise::validate() { return m_view->validate(); }

void IndirectSymmetrise::runClicked() { runTab(); }

/**
 * Handles saving of workspace
 */
void IndirectSymmetrise::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName), QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatch();
}

void IndirectSymmetrise::run() {
  m_view->setRawPlotWatchADS(false);

  QString workspaceName = m_view->getInputName();
  QString outputWorkspaceName = workspaceName.left(workspaceName.length() - 4) + "_sym" + workspaceName.right(4);

  double e_min = m_view->getEMin();
  double e_max = m_view->getEMax();

  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  symmetriseAlg->setProperty("XMin", e_min);
  symmetriseAlg->setProperty("XMax", e_max);
  symmetriseAlg->setProperty("OutputWorkspace", outputWorkspaceName.toStdString());
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  m_batchAlgoRunner->addAlgorithm(symmetriseAlg);

  // Set the workspace name for Python script export
  m_pythonExportWsName = outputWorkspaceName.toStdString();

  // Handle algorithm completion signal
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));

  // Execute algorithm on seperate thread
  m_batchAlgoRunner->executeBatchAsync();
}

/**
 * Handle plotting result workspace.
 *
 * @param error If the algorithm failed
 */
void IndirectSymmetrise::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
  m_view->setRawPlotWatchADS(true);

  if (error)
    return;

  setOutputPlotOptionsWorkspaces({m_pythonExportWsName});

  // Enable save and plot
  m_view->enableSave(true);
}

/**
 * Plots a new workspace in the mini plot when it is loaded form the data
 *selector.
 *
 * @param workspaceName Name of the workspace that has been loaded
 */
void IndirectSymmetrise::plotNewData(QString const &workspaceName) { m_view->plotNewData(workspaceName); }

/**
 * Handles a request to preview the symmetrise.
 *
 * Runs Symmetrise on the current spectrum and plots in preview mini plot.
 *
 * @see IndirectSymmetrise::previewAlgDone()
 */
void IndirectSymmetrise::preview() {
  // Handle algorithm completion signal
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(previewAlgDone(bool)));

  m_view->setRawPlotWatchADS(false);

  // Do nothing if no data has been laoded
  QString workspaceName = m_view->getInputName();
  if (workspaceName.isEmpty())
    return;

  double e_min = m_view->getEMin();
  double e_max = m_view->getEMax();

  long spectrumNumber = static_cast<long>(m_view->getPreviewSpec());
  std::vector<long> spectraRange(2, spectrumNumber);

  // Run the algorithm on the preview spectrum only
  IAlgorithm_sptr symmetriseAlg = AlgorithmManager::Instance().create("Symmetrise", -1);
  symmetriseAlg->initialize();
  symmetriseAlg->setProperty("InputWorkspace", workspaceName.toStdString());
  symmetriseAlg->setProperty("XMin", e_min);
  symmetriseAlg->setProperty("XMax", e_max);
  symmetriseAlg->setProperty("SpectraRange", spectraRange);
  symmetriseAlg->setProperty("OutputWorkspace", "__Symmetrise_temp");
  symmetriseAlg->setProperty("OutputPropertiesTable", "__SymmetriseProps_temp");

  runAlgorithm(symmetriseAlg);

  // Now enable the run function
  m_view->enableRun(true);
}

/**
 * Handles completion of the preview algorithm.
 *
 * @param error If the algorithm failed
 */
void IndirectSymmetrise::previewAlgDone(bool error) {
  if (error)
    return;
  m_view->previewAlgDone();
  // Don't want this to trigger when the algorithm is run for all spectra
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(previewAlgDone(bool)));
}

void IndirectSymmetrise::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Symmetrise");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

} // namespace CustomInterfaces
} // namespace MantidQt
