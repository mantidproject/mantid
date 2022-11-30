// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationSymmetriseTab.h"
#include "IndirectDataValidationHelper.h"

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/Logger.h"
#include "MantidQtWidgets/Common/UserInputValidator.h"
#include "MantidQtWidgets/Plotting/SingleSelector.h"

using namespace IndirectDataValidationHelper;
using namespace Mantid::API;

namespace {
Mantid::Kernel::Logger g_log("InelasticDataManipulationSymmetriseTab");
} // namespace

namespace MantidQt {
using MantidWidgets::AxisID;
namespace CustomInterfaces {
//----------------------------------------------------------------------------------------------
/** Constructor
 */
InelasticDataManipulationSymmetriseTab::InelasticDataManipulationSymmetriseTab(QWidget *parent)
    : InelasticDataManipulationTab(parent), m_adsInstance(Mantid::API::AnalysisDataService::Instance()),
      m_view(std::make_unique<InelasticDataManipulationSymmetriseTabView>(parent)),
      m_model(std::make_unique<InelasticDataManipulationSymmetriseTabModel>()) {
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra));

  // SIGNAL/SLOT CONNECTIONS
  // Preview symmetrise
  connect(m_view.get(), SIGNAL(valueChanged(QtProperty *, double)), this,
          SLOT(handleValueChanged(QtProperty *, double)));
  connect(m_view.get(), SIGNAL(dataReady(QString const &)), this, SLOT(handleDataReady(QString const &)));
  connect(m_view.get(), SIGNAL(previewClicked()), this, SLOT(preview()));
  // Handle running, plotting and saving
  connect(m_view.get(), SIGNAL(runClicked()), this, SLOT(runClicked()));
  connect(m_view.get(), SIGNAL(saveClicked()), this, SLOT(saveClicked()));
  m_view->setDefaults();
}

InelasticDataManipulationSymmetriseTab::~InelasticDataManipulationSymmetriseTab() {
  m_propTrees["SymmPropTree"]->unsetFactoryForManager(m_dblManager);
}

void InelasticDataManipulationSymmetriseTab::setup() {}

bool InelasticDataManipulationSymmetriseTab::validate() { return m_view->validate(); }

void InelasticDataManipulationSymmetriseTab::runClicked() { runTab(); }

/**
 * Handles saving of workspace
 */
void InelasticDataManipulationSymmetriseTab::saveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName), QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatch();
}

void InelasticDataManipulationSymmetriseTab::run() {
  m_view->setRawPlotWatchADS(false);

  auto const outputWorkspaceName = m_model->setupSymmetriseAlgorithm(m_batchAlgoRunner);

  // Set the workspace name for Python script export
  m_pythonExportWsName = outputWorkspaceName;

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
void InelasticDataManipulationSymmetriseTab::algorithmComplete(bool error) {
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(algorithmComplete(bool)));
  m_view->setRawPlotWatchADS(true);
  if (error)
    return;
  setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
  // Enable save and plot
  m_view->enableSave(true);
}

/**
 * Handles a request to preview the symmetrise.
 *
 * Runs Symmetrise on the current spectrum and plots in preview mini plot.
 *
 * @see InelasticDataManipulationSymmetriseTab::previewAlgDone()
 */
void InelasticDataManipulationSymmetriseTab::preview() {
  // Handle algorithm completion signal
  connect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(previewAlgDone(bool)));

  m_view->setRawPlotWatchADS(false);

  // Do nothing if no data has been laoded
  QString workspaceName = m_view->getInputName();
  if (workspaceName.isEmpty())
    return;

  long spectrumNumber = static_cast<long>(m_view->getPreviewSpec());
  std::vector<long> spectraRange(2, spectrumNumber);

  m_model->setupPreviewAlgorithm(m_batchAlgoRunner, spectraRange);

  // There should never really be unexecuted algorithms in the queue, but it is
  // worth warning in case of possible weirdness
  size_t batchQueueLength = m_batchAlgoRunner->queueLength();
  if (batchQueueLength > 0)
    g_log.warning() << "Batch queue already contains " << batchQueueLength << " algorithms!\n";

  m_batchAlgoRunner->executeBatchAsync();

  // Now enable the run function
  m_view->enableRun(true);
}

/**
 * Handles completion of the preview algorithm.
 *
 * @param error If the algorithm failed
 */
void InelasticDataManipulationSymmetriseTab::previewAlgDone(bool error) {
  if (error)
    return;
  m_view->previewAlgDone();
  // Don't want this to trigger when the algorithm is run for all spectra
  disconnect(m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this, SLOT(previewAlgDone(bool)));
}

void InelasticDataManipulationSymmetriseTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Symmetrise");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

void InelasticDataManipulationSymmetriseTab::handleValueChanged(QtProperty *prop, double value) {
  if (prop->propertyName() == "Spectrum No") {
    m_view->replotNewSpectrum(value);
  } else if (prop->propertyName() == "EMin") {
    m_view->verifyERange(prop, value);
    m_model->setEMin(m_view->getEMin());
  } else if (prop->propertyName() == "EMax") {
    m_view->verifyERange(prop, value);
    m_model->setEMax(m_view->getEMax());
  }
}

void InelasticDataManipulationSymmetriseTab::handleDataReady(QString const &dataName) {
  if (m_view->validate()) {
    m_view->plotNewData(dataName);
  }
  m_model->setWorkspaceName(dataName);
}

} // namespace CustomInterfaces
} // namespace MantidQt
