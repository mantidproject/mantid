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
#include "MantidQtWidgets/Common/HelpWindow.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

using API::IConfiguredAlgorithm_sptr;

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
    : m_view(view), m_mainPresenter(),
      m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(std::move(eventPresenter)),
      m_experimentPresenter(std::move(experimentPresenter)),
      m_instrumentPresenter(std::move(instrumentPresenter)),
      m_savePresenter(std::move(savePresenter)),
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

void BatchPresenter::initInstrumentList() {
  m_runsPresenter->initInstrumentList();
}

bool BatchPresenter::requestClose() const { return true; }

void BatchPresenter::notifyChangeInstrumentRequested(
    const std::string &instrumentName) {
  m_mainPresenter->notifyChangeInstrumentRequested(instrumentName);
}

void BatchPresenter::notifyInstrumentChanged(
    const std::string &instrumentName) {
  m_runsPresenter->notifyInstrumentChanged(instrumentName);
  m_experimentPresenter->notifyInstrumentChanged(instrumentName);
  m_instrumentPresenter->notifyInstrumentChanged(instrumentName);
}

void BatchPresenter::notifyUpdateInstrumentRequested() {
  m_mainPresenter->notifyUpdateInstrumentRequested();
}

void BatchPresenter::notifySettingsChanged() { settingsChanged(); }

void BatchPresenter::notifyResumeReductionRequested() { resumeReduction(); }

void BatchPresenter::notifyPauseReductionRequested() { pauseReduction(); }

void BatchPresenter::notifyResumeAutoreductionRequested() {
  resumeAutoreduction();
}

void BatchPresenter::notifyPauseAutoreductionRequested() {
  pauseAutoreduction();
}

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

  notifyReductionPaused();
}

void BatchPresenter::notifyBatchCancelled() {
  notifyReductionPaused();
  // We also stop autoreduction if the user has cancelled
  notifyAutoreductionPaused();
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
  m_jobRunner->notifyReductionResumed();
  // Get the algorithms to process
  auto algorithms = m_jobRunner->getAlgorithms();
  if (algorithms.size() < 1) {
    notifyReductionPaused();
    return;
  }
  // Start processing
  notifyReductionResumed();
  startBatch(std::move(algorithms));
}

void BatchPresenter::notifyReductionResumed() {
  // Notify child presenters
  m_savePresenter->notifyReductionResumed();
  m_eventPresenter->notifyReductionResumed();
  m_experimentPresenter->notifyReductionResumed();
  m_instrumentPresenter->notifyReductionResumed();
  m_runsPresenter->notifyReductionResumed();
  m_mainPresenter->notifyAnyBatchReductionResumed();
}

void BatchPresenter::pauseReduction() { m_view->cancelAlgorithmQueue(); }

void BatchPresenter::notifyReductionPaused() {
  // Update the model
  m_jobRunner->notifyReductionPaused();
  // Notify child presenters
  m_savePresenter->notifyReductionPaused();
  m_eventPresenter->notifyReductionPaused();
  m_experimentPresenter->notifyReductionPaused();
  m_instrumentPresenter->notifyReductionPaused();
  m_runsPresenter->notifyReductionPaused();
  m_mainPresenter->notifyAnyBatchReductionPaused();
  // If autoreducing, notify
  if (isAutoreducing())
    notifyAutoreductionCompleted();
}

void BatchPresenter::resumeAutoreduction() {
  // Update the model first to ensure the autoprocessing flag is set
  m_jobRunner->notifyAutoreductionResumed();
  // The runs presenter starts autoreduction. This sets off a search to find
  // new runs, if there are any. When the search completes, we'll receive
  // a separate callback to notifyReductionResumed.
  if (m_runsPresenter->resumeAutoreduction())
    notifyAutoreductionResumed();
  else
    m_jobRunner->notifyAutoreductionPaused();
}

void BatchPresenter::notifyAutoreductionResumed() {
  // Notify child presenters
  m_savePresenter->notifyAutoreductionResumed();
  m_eventPresenter->notifyAutoreductionResumed();
  m_experimentPresenter->notifyAutoreductionResumed();
  m_instrumentPresenter->notifyAutoreductionResumed();
  m_runsPresenter->notifyAutoreductionResumed();

  m_runsPresenter->notifyRowStateChanged();
  m_mainPresenter->notifyAnyBatchAutoreductionResumed();
}

void BatchPresenter::pauseAutoreduction() {
  // Update the model
  m_jobRunner->notifyAutoreductionPaused();
  // Stop all processing
  pauseReduction();
  // Notify child presenters
  notifyAutoreductionPaused();
}

void BatchPresenter::notifyAutoreductionPaused() {
  // Notify child presenters
  m_savePresenter->notifyAutoreductionPaused();
  m_eventPresenter->notifyAutoreductionPaused();
  m_experimentPresenter->notifyAutoreductionPaused();
  m_instrumentPresenter->notifyAutoreductionPaused();
  m_runsPresenter->notifyAutoreductionPaused();

  m_mainPresenter->notifyAnyBatchAutoreductionPaused();
}

void BatchPresenter::autoreductionCompleted() {
  m_runsPresenter->autoreductionCompleted();
  m_runsPresenter->notifyRowStateChanged();
}

void BatchPresenter::notifyAnyBatchReductionResumed() {
  m_runsPresenter->notifyAnyBatchReductionResumed();
}

void BatchPresenter::notifyAnyBatchReductionPaused() {
  m_runsPresenter->notifyAnyBatchReductionPaused();
}

void BatchPresenter::notifyAnyBatchAutoreductionResumed() {
  m_runsPresenter->notifyAnyBatchAutoreductionResumed();
}

void BatchPresenter::notifyAnyBatchAutoreductionPaused() {
  m_runsPresenter->notifyAnyBatchAutoreductionPaused();
}

Mantid::Geometry::Instrument_const_sptr BatchPresenter::instrument() const {
  return m_mainPresenter->instrument();
}

std::string BatchPresenter::instrumentName() const {
  return m_mainPresenter->instrumentName();
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
   Checks whether or not processing is currently running in any batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isAnyBatchProcessing() const {
  return m_mainPresenter->isAnyBatchProcessing();
}

/**
   Checks whether or not autoprocessing is currently running in any batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being autoprocessed
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
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
