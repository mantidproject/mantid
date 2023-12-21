// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "InelasticDataManipulationSymmetriseTab.h"
#include "Common/IndirectDataValidationHelper.h"

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
InelasticDataManipulationSymmetriseTab::InelasticDataManipulationSymmetriseTab(QWidget *parent, ISymmetriseView *view)
    : InelasticDataManipulationTab(parent), m_adsInstance(Mantid::API::AnalysisDataService::Instance()), m_view(view),
      m_model(std::make_unique<InelasticDataManipulationSymmetriseTabModel>()), m_isPreview(false) {
  m_view->subscribePresenter(this);
  setOutputPlotOptionsPresenter(
      std::make_unique<IndirectPlotOptionsPresenter>(m_view->getPlotOptions(), PlotWidget::Spectra));

  m_model->setIsPositiveReflect(true);
  m_view->setDefaults();
}

InelasticDataManipulationSymmetriseTab::~InelasticDataManipulationSymmetriseTab() {
  m_propTrees["SymmPropTree"]->unsetFactoryForManager(m_dblManager);
}

void InelasticDataManipulationSymmetriseTab::setup() {}

bool InelasticDataManipulationSymmetriseTab::validate() { return m_view->validate(); }

void InelasticDataManipulationSymmetriseTab::handleRunOrPreviewClicked(bool isPreview) {
  setIsPreview(isPreview);
  runTab();
}

/**
 * Handles saving of workspace
 */
void InelasticDataManipulationSymmetriseTab::handleSaveClicked() {
  if (checkADSForPlotSaveWorkspace(m_pythonExportWsName, false))
    addSaveWorkspaceToQueue(QString::fromStdString(m_pythonExportWsName), QString::fromStdString(m_pythonExportWsName));
  m_batchAlgoRunner->executeBatch();
}

/** Handles running the algorithm either from run button or preview button. Set with m_isPreview flag.
 *
 */
void InelasticDataManipulationSymmetriseTab::run() {
  m_view->setRawPlotWatchADS(false);

  // There should never really be unexecuted algorithms in the queue, but it is
  // worth warning in case of possible weirdness
  size_t batchQueueLength = m_batchAlgoRunner->queueLength();
  if (batchQueueLength > 0)
    g_log.warning() << "Batch queue already contains " << batchQueueLength << " algorithms!\n";

  // Return if no data has been loaded
  auto const dataWorkspaceName = m_view->getDataName();
  if (dataWorkspaceName.empty())
    return;
  // Return if E range is incorrect
  if (!m_view->verifyERange(dataWorkspaceName))
    return;

  if (m_isPreview) {
    long spectrumNumber = static_cast<long>(m_view->getPreviewSpec());
    std::vector<long> spectraRange(2, spectrumNumber);

    m_model->setupPreviewAlgorithm(m_batchAlgoRunner, spectraRange);
  } else {
    auto const outputWorkspaceName = m_model->setupSymmetriseAlgorithm(m_batchAlgoRunner);
    // Set the workspace name for Python script export
    m_pythonExportWsName = outputWorkspaceName;
  }

  // Execute the algorithm(s) on a separated thread
  m_batchAlgoRunner->executeBatchAsync();
  // Now enable the run function
  m_view->enableRun(true);
}

/**
 * Handle plotting result or preview workspace.
 *
 * @param error If the algorithm failed
 */
void InelasticDataManipulationSymmetriseTab::runComplete(bool error) {
  if (error)
    return;

  if (m_isPreview) {
    m_view->previewAlgDone();
  } else {
    setOutputPlotOptionsWorkspaces({m_pythonExportWsName});
    // Enable save and plot
    m_view->enableSave(true);
  }
  m_view->setRawPlotWatchADS(true);
}

void InelasticDataManipulationSymmetriseTab::setFileExtensionsByName(bool filter) {
  QStringList const noSuffixes{""};
  auto const tabName("Symmetrise");
  m_view->setFBSuffixes(filter ? getSampleFBSuffixes(tabName) : getExtensions(tabName));
  m_view->setWSSuffixes(filter ? getSampleWSSuffixes(tabName) : noSuffixes);
}

void InelasticDataManipulationSymmetriseTab::handleReflectTypeChanged(int value) {
  m_model->setIsPositiveReflect(value == 0);
}

void InelasticDataManipulationSymmetriseTab::handleDoubleValueChanged(std::string const &propName, double value) {
  if (propName == "Spectrum No") {
    m_view->replotNewSpectrum(value);
  } else {
    m_view->updateRangeSelectors(propName, value);
    if (propName == "Elow") {
      m_model->getIsPositiveReflect() ? m_model->setEMin(value) : m_model->setEMax((-1) * value);
    } else if (propName == "Ehigh") {
      m_model->getIsPositiveReflect() ? m_model->setEMax(value) : m_model->setEMin((-1) * value);
    }
  }
}

void InelasticDataManipulationSymmetriseTab::handleDataReady(std::string const &dataName) {
  if (m_view->validate()) {
    m_view->plotNewData(dataName);
  }
  m_model->setWorkspaceName(dataName);
}

void InelasticDataManipulationSymmetriseTab::setIsPreview(bool preview) { m_isPreview = preview; }

} // namespace CustomInterfaces
} // namespace MantidQt
