// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchPresenter.h"
#include "BatchJobRunner.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/MainWindow/IMainWindowPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IBatchView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm_fwd.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"

namespace MantidQt {
namespace CustomInterfaces {

using API::IConfiguredAlgorithm_sptr;
using Mantid::API::AlgorithmManager;
using Mantid::API::IAlgorithm_sptr;
using Mantid::API::MatrixWorkspace_sptr;

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");
}

/** Constructor
 * @param view :: [input] The view we are managing
 * @param model :: [input] The reduction configuration model
 * @param runsPresenter :: [input] A pointer to the 'Runs' tab presenter
 * @param eventPresenter :: [input] A pointer to the 'Event Handling' tab
 * presenter
 * @param experimentPresenter :: [input] A pointer to the 'Experiment' tab
 * presenter
 * @param instrumentPresenter :: [input] A pointer to the 'Instrument' tab
 * presenter
 * @param savePresenter :: [input] A pointer to the 'Save ASCII' tab presenter
 */
BatchPresenter::BatchPresenter(
    IBatchView *view, Batch model,
    std::unique_ptr<IRunsPresenter> runsPresenter,
    std::unique_ptr<IEventPresenter> eventPresenter,
    std::unique_ptr<IExperimentPresenter> experimentPresenter,
    std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
    std::unique_ptr<ISavePresenter> savePresenter)
    : m_view(view), m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(std::move(eventPresenter)),
      m_experimentPresenter(std::move(experimentPresenter)),
      m_instrumentPresenter(std::move(instrumentPresenter)),
      m_savePresenter(std::move(savePresenter)), m_instrument(),
      m_jobRunner(new BatchJobRunner(std::move(model))) {

  m_view->subscribe(this);

  // Tell the tab presenters that this is going to be the main presenter
  m_savePresenter->acceptMainPresenter(this);
  m_eventPresenter->acceptMainPresenter(this);
  m_experimentPresenter->acceptMainPresenter(this);
  m_instrumentPresenter->acceptMainPresenter(this);
  m_runsPresenter->acceptMainPresenter(this);

  observePostDelete();
  observeRename();
  observeADSClear();
}

/** Accept a main presenter
 * @param mainPresenter :: [input] A main presenter
 */
void BatchPresenter::acceptMainPresenter(IMainWindowPresenter *mainPresenter) {
  m_mainPresenter = mainPresenter;
}

bool BatchPresenter::requestClose() const { return true; }

void BatchPresenter::notifyInstrumentChanged(
    const std::string &instrumentName) {
  instrumentChanged(instrumentName);
}

void BatchPresenter::notifyRestoreDefaultsRequested() {
  // We need to reload the instrument parameters file so that we can get
  // up-to-date defaults
  updateInstrument(m_instrument->getName());
}

void BatchPresenter::notifySettingsChanged() { settingsChanged(); }

void BatchPresenter::notifyReductionResumed() { resumeReduction(); }

void BatchPresenter::notifyReductionPaused() { pauseReduction(); }

void BatchPresenter::notifyAutoreductionResumed() { resumeAutoreduction(); }

void BatchPresenter::notifyAutoreductionPaused() { pauseAutoreduction(); }

void BatchPresenter::notifyAutoreductionCompleted() {
  autoreductionCompleted();
}

void BatchPresenter::notifyBatchComplete(bool error) {
  UNUSED_ARG(error);

  // Continue processing the next batch of algorithms, if there is more to do
  auto algorithms = m_jobRunner->getAlgorithms();
  if (algorithms.size() > 0) {
    startBatch(std::move(algorithms));
    return;
  }

  reductionPaused();
}

void BatchPresenter::notifyBatchCancelled() {
  reductionPaused();
  // We also stop autoreduction if the user has cancelled
  autoreductionPaused();
}

void BatchPresenter::notifyAlgorithmStarted(
    IConfiguredAlgorithm_sptr algorithm) {
  auto const &item = m_jobRunner->algorithmStarted(algorithm);
  m_runsPresenter->notifyRowOutputsChanged(item);
  m_runsPresenter->notifyRowStateChanged(item);
}

void BatchPresenter::notifyAlgorithmComplete(
    IConfiguredAlgorithm_sptr algorithm) {
  auto const &item = m_jobRunner->algorithmComplete(algorithm);
  m_runsPresenter->notifyRowOutputsChanged(item);
  m_runsPresenter->notifyRowStateChanged(item);
  /// TODO Longer term it would probably be better if algorithms took care
  /// of saving their outputs so we could remove this callback
  if (m_savePresenter->shouldAutosave()) {
    auto const workspaces =
        m_jobRunner->algorithmOutputWorkspacesToSave(algorithm);
    m_savePresenter->saveWorkspaces(workspaces);
  }
}

void BatchPresenter::notifyAlgorithmError(IConfiguredAlgorithm_sptr algorithm,
                                          std::string const &message) {
  auto const &item = m_jobRunner->algorithmError(algorithm, message);
  m_runsPresenter->notifyRowOutputsChanged(item);
  m_runsPresenter->notifyRowStateChanged(item);
}

/** Start processing the next batch of algorithms.
 * @returns : true if processing was started, false if there was nothing to do
 */
bool BatchPresenter::startBatch(
    std::deque<IConfiguredAlgorithm_sptr> algorithms) {
  m_view->clearAlgorithmQueue();
  m_view->setAlgorithmQueue(std::move(algorithms));
  m_view->executeAlgorithmQueue();
  return true;
}

void BatchPresenter::resumeReduction() {
  // Update the model
  m_jobRunner->reductionResumed();
  // Get the algorithms to process
  auto algorithms = m_jobRunner->getAlgorithms();
  if (algorithms.size() < 1) {
    reductionPaused();
    return;
  }
  // Start processing
  reductionResumed();
  startBatch(std::move(algorithms));
}

void BatchPresenter::reductionResumed() {
  // Notify child presenters
  m_savePresenter->reductionResumed();
  m_eventPresenter->reductionResumed();
  m_experimentPresenter->reductionResumed();
  m_instrumentPresenter->reductionResumed();
  m_runsPresenter->reductionResumed();
}

void BatchPresenter::pauseReduction() { m_view->cancelAlgorithmQueue(); }

void BatchPresenter::reductionPaused() {
  // Update the model
  m_jobRunner->reductionPaused();
  // Notify child presenters
  m_savePresenter->reductionPaused();
  m_eventPresenter->reductionPaused();
  m_experimentPresenter->reductionPaused();
  m_instrumentPresenter->reductionPaused();
  m_runsPresenter->reductionPaused();
  // If autoreducing, notify
  if (isAutoreducing())
    notifyAutoreductionCompleted();
}

void BatchPresenter::resumeAutoreduction() {
  // Update the model first to ensure the autoprocessing flag is set
  m_jobRunner->autoreductionResumed();
  // The runs presenter starts autoreduction. This sets off a search to find
  // new runs, if there are any. When the search completes, we'll receive
  // a separate callback to reductionResumed.
  if (m_runsPresenter->resumeAutoreduction())
    autoreductionResumed();
  else
    m_jobRunner->autoreductionPaused();
}

void BatchPresenter::autoreductionResumed() {
  // Notify child presenters
  m_savePresenter->autoreductionResumed();
  m_eventPresenter->autoreductionResumed();
  m_experimentPresenter->autoreductionResumed();
  m_instrumentPresenter->autoreductionResumed();
  m_runsPresenter->autoreductionResumed();

  m_runsPresenter->notifyRowStateChanged();
  m_mainPresenter->notifyAutoreductionResumed();
}

void BatchPresenter::pauseAutoreduction() {
  // Update the model
  m_jobRunner->autoreductionPaused();
  // Stop all processing
  pauseReduction();
  // Notify child presenters
  autoreductionPaused();
}

void BatchPresenter::autoreductionPaused() {
  // Notify child presenters
  m_savePresenter->autoreductionPaused();
  m_eventPresenter->autoreductionPaused();
  m_experimentPresenter->autoreductionPaused();
  m_instrumentPresenter->autoreductionPaused();
  m_runsPresenter->autoreductionPaused();

  m_mainPresenter->notifyAutoreductionResumed();
}

void BatchPresenter::autoreductionCompleted() {
  m_runsPresenter->autoreductionCompleted();
  m_runsPresenter->notifyRowStateChanged();
}

void BatchPresenter::anyBatchAutoreductionResumed() {
  m_runsPresenter->anyBatchAutoreductionResumed();
}

void BatchPresenter::anyBatchAutoreductionPaused() {
  m_runsPresenter->anyBatchAutoreductionPaused();
}

void BatchPresenter::instrumentChanged(const std::string &instrumentName) {
  updateInstrument(instrumentName);
  m_runsPresenter->instrumentChanged(instrumentName);
  m_experimentPresenter->instrumentChanged(instrumentName);
  m_instrumentPresenter->instrumentChanged(instrumentName);
}

void BatchPresenter::updateInstrument(const std::string &instrumentName) {
  Mantid::Kernel::ConfigService::Instance().setString("default.instrument",
                                                      instrumentName);
  g_log.information() << "Instrument changed to " << instrumentName;

  // Load a workspace for this instrument so we can get the actual instrument
  auto loadAlg =
      AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->initialize();
  loadAlg->setProperty("InstrumentName", instrumentName);
  loadAlg->setProperty("OutputWorkspace",
                       "__Reflectometry_GUI_Empty_Instrument");
  loadAlg->execute();
  MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");
  m_instrument = instWorkspace->getInstrument();
}

Mantid::Geometry::Instrument_const_sptr BatchPresenter::instrument() const {
  return m_instrument;
}

std::string BatchPresenter::instrumentName() const {
  if (m_instrument)
    return m_instrument->getName();

  return std::string();
}

void BatchPresenter::settingsChanged() { m_runsPresenter->settingsChanged(); }

/**
   Checks whether or not data is currently being processed in this batch
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isProcessing() const {
  return m_jobRunner->isProcessing();
}

/**
   Checks whether or not autoprocessing is currently running in this batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isAutoreducing() const {
  return m_jobRunner->isAutoreducing();
}

/**
   Checks whether or not autoprocessing is currently running in this batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isAnyBatchAutoreducing() const {
  return m_mainPresenter->isAnyBatchAutoreducing();
}

/** Get the percent of jobs that have been completed out of the current
    processing list
 */
int BatchPresenter::percentComplete() const {
  return m_jobRunner->percentComplete();
}

AlgorithmRuntimeProps BatchPresenter::rowProcessingProperties() const {
  return m_jobRunner->rowProcessingProperties();
}

void BatchPresenter::postDeleteHandle(const std::string &wsName) {
  auto const item = m_jobRunner->notifyWorkspaceDeleted(wsName);
  m_runsPresenter->notifyRowOutputsChanged(item);
  m_runsPresenter->notifyRowStateChanged(item);
}

void BatchPresenter::renameHandle(const std::string &oldName,
                                  const std::string &newName) {
  auto const item = m_jobRunner->notifyWorkspaceRenamed(oldName, newName);
  m_runsPresenter->notifyRowOutputsChanged(item);
  m_runsPresenter->notifyRowStateChanged(item);
}

void BatchPresenter::clearADSHandle() {
  m_jobRunner->notifyAllWorkspacesDeleted();
  m_runsPresenter->notifyRowOutputsChanged();
  m_runsPresenter->notifyRowStateChanged();
}
} // namespace CustomInterfaces
} // namespace MantidQt
