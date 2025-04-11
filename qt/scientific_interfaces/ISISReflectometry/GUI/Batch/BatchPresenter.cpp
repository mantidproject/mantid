// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchPresenter.h"
#include "BatchJobManager.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/MainWindow/IMainWindowPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IBatchView.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/IMessageHandler.h"
#include "Reduction/RowExceptions.h"
#include <memory>

namespace {
Mantid::Kernel::Logger g_log("Reflectometry Batch Presenter");
} // namespace

namespace MantidQt::CustomInterfaces::ISISReflectometry {

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
 * @param savePresenter :: [input] A pointer to the 'Save' tab presenter
 */
BatchPresenter::BatchPresenter(
    IBatchView *view, std::unique_ptr<IBatch> model, std::unique_ptr<API::IJobRunner> jobRunner,
    std::unique_ptr<IRunsPresenter> runsPresenter, std::unique_ptr<IEventPresenter> eventPresenter,
    std::unique_ptr<IExperimentPresenter> experimentPresenter,
    std::unique_ptr<IInstrumentPresenter> instrumentPresenter, std::unique_ptr<ISavePresenter> savePresenter,
    std::unique_ptr<IPreviewPresenter> previewPresenter, MantidQt::MantidWidgets::IMessageHandler *messageHandler)
    : m_view(view), m_model(std::move(model)), m_mainPresenter(), m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(std::move(eventPresenter)), m_experimentPresenter(std::move(experimentPresenter)),
      m_instrumentPresenter(std::move(instrumentPresenter)), m_savePresenter(std::move(savePresenter)),
      m_previewPresenter(std::move(previewPresenter)), m_unsavedBatchFlag(false), m_jobRunner(std::move(jobRunner)),
      m_messageHandler(messageHandler), m_jobManager(std::make_unique<BatchJobManager>(*m_model)) {

  m_jobRunner->subscribe(this);

  // Tell the tab presenters that this is going to be the main presenter
  m_savePresenter->acceptMainPresenter(this);
  m_eventPresenter->acceptMainPresenter(this);
  m_experimentPresenter->acceptMainPresenter(this);
  m_instrumentPresenter->acceptMainPresenter(this);
  m_runsPresenter->acceptMainPresenter(this);
  m_previewPresenter->acceptMainPresenter(this);

  m_unsavedBatchFlag = false;

  observePostDelete();
  observeRename();
  observeADSClear();
}

/** Accept a main presenter
 * @param mainPresenter :: [input] A main presenter
 */
void BatchPresenter::acceptMainPresenter(IMainWindowPresenter *mainPresenter) { m_mainPresenter = mainPresenter; }

std::string BatchPresenter::initInstrumentList(const std::string &selectedInstrument) {
  return m_runsPresenter->initInstrumentList(selectedInstrument);
}

bool BatchPresenter::requestClose() const { return true; }

void BatchPresenter::notifyChangeInstrumentRequested(const std::string &instrumentName) {
  m_mainPresenter->notifyChangeInstrumentRequested(instrumentName);
}

void BatchPresenter::notifyInstrumentChanged(const std::string &instrumentName) {
  m_runsPresenter->notifyInstrumentChanged(instrumentName);
  m_experimentPresenter->notifyInstrumentChanged(instrumentName);
  m_instrumentPresenter->notifyInstrumentChanged(instrumentName);
}

void BatchPresenter::notifyUpdateInstrumentRequested() { m_mainPresenter->notifyUpdateInstrumentRequested(); }

void BatchPresenter::notifySettingsChanged() { settingsChanged(); }

void BatchPresenter::notifyResumeReductionRequested() { resumeReduction(); }

void BatchPresenter::notifyPauseReductionRequested() { pauseReduction(); }

void BatchPresenter::notifyResumeAutoreductionRequested() { resumeAutoreduction(); }

void BatchPresenter::notifyPauseAutoreductionRequested() { pauseAutoreduction(); }

void BatchPresenter::notifyAutoreductionCompleted() { autoreductionCompleted(); }

void BatchPresenter::notifyBatchComplete(bool error) {
  UNUSED_ARG(error);

  // Continue processing the next batch of algorithms, if there is more to do
  auto algorithms = m_jobManager->getAlgorithms();
  if (algorithms.size() > 0) {
    notifyReductionResumed();
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

void BatchPresenter::notifyAlgorithmStarted(IConfiguredAlgorithm_sptr &algorithm) {
  auto item = m_jobManager->getRunsTableItem(algorithm);
  if (!item) {
    return;
  }
  m_jobManager->algorithmStarted(algorithm);
  m_runsPresenter->notifyRowModelChanged(item.value());
}

void BatchPresenter::notifyAlgorithmComplete(IConfiguredAlgorithm_sptr &algorithm) {
  auto item = m_jobManager->getRunsTableItem(algorithm);
  if (!item) {
    return;
  }
  m_jobManager->algorithmComplete(algorithm);
  m_runsPresenter->notifyRowModelChanged(item.value());
  /// TODO Longer term it would probably be better if algorithms took care
  /// of saving their outputs so we could remove this callback
  if (m_savePresenter->shouldAutosave()) {
    auto const workspaces =
        m_jobManager->algorithmOutputWorkspacesToSave(algorithm, m_savePresenter->shouldAutosaveGroupRows());

    if (!workspaces.empty()) {
      try {
        m_savePresenter->saveWorkspaces(workspaces, true);
      } catch (std::runtime_error const &e) {
        g_log.error(e.what());
      } catch (std::exception const &e) {
        g_log.error(e.what());
      } catch (...) {
        g_log.error("Unknown error while saving workspaces.");
      }
    }
  }
}

void BatchPresenter::notifyAlgorithmError(IConfiguredAlgorithm_sptr &algorithm, std::string const &message) {
  auto item = m_jobManager->getRunsTableItem(algorithm);
  if (!item) {
    return;
  }
  m_jobManager->algorithmError(algorithm, message);
  m_runsPresenter->notifyRowModelChanged(item.value());
}

/** Start processing the next batch of algorithms.
 * @returns : true if processing was started, false if there was nothing to do
 */
bool BatchPresenter::startBatch(std::deque<IConfiguredAlgorithm_sptr> algorithms) {
  m_jobRunner->clearAlgorithmQueue();
  m_jobRunner->setAlgorithmQueue(std::move(algorithms));
  m_jobRunner->executeAlgorithmQueue();
  return true;
}

void BatchPresenter::resumeReduction() {
  if (!m_experimentPresenter->hasValidSettings()) {
    m_messageHandler->giveUserCritical(
        "One or more of the experiment settings is invalid. Please check the Experiment Settings tab.",
        "Processing Error");
    return;
  }
  // Update the model
  m_jobManager->notifyReductionResumed();
  // Get the algorithms to process
  auto algorithms = m_jobManager->getAlgorithms();
  if (algorithms.size() < 1 || (m_jobManager->getProcessAll() && m_mainPresenter->isProcessAllPrevented()) ||
      (m_jobManager->getProcessPartial() && m_mainPresenter->isProcessPartialGroupPrevented())) {
    notifyReductionPaused();
    return;
  }
  // Start processing
  notifyReductionResumed();
  startBatch(std::move(algorithms));
}

void BatchPresenter::notifyReductionResumed() {
  // Notify child presenters
  m_previewPresenter->notifyReductionResumed();
  m_savePresenter->notifyReductionResumed();
  m_eventPresenter->notifyReductionResumed();
  m_experimentPresenter->notifyReductionResumed();
  m_instrumentPresenter->notifyReductionResumed();
  m_runsPresenter->notifyReductionResumed();
  m_mainPresenter->notifyAnyBatchReductionResumed();
}

void BatchPresenter::pauseReduction() { m_jobRunner->cancelAlgorithmQueue(); }

void BatchPresenter::notifyReductionPaused() {
  // Update the model
  m_jobManager->notifyReductionPaused();
  // Notify child presenters
  m_previewPresenter->notifyReductionPaused();
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
  if (!m_experimentPresenter->hasValidSettings()) {
    m_messageHandler->giveUserCritical(
        "One or more of the experiment settings is invalid. Please check the Experiment Settings tab.",
        "Processing Error");
    return;
  }
  // Update the model first to ensure the autoprocessing flag is set
  m_jobManager->notifyAutoreductionResumed();
  // The runs presenter starts autoreduction. This sets off a search to find
  // new runs, if there are any. When the search completes, we'll receive
  // a separate callback to notifyReductionResumed.
  if (m_runsPresenter->resumeAutoreduction())
    notifyAutoreductionResumed();
  else
    m_jobManager->notifyAutoreductionPaused();
}

void BatchPresenter::notifyAutoreductionResumed() {
  // Notify child presenters
  m_previewPresenter->notifyAutoreductionResumed();
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
  m_jobManager->notifyAutoreductionPaused();
  // Stop all processing
  pauseReduction();
  // Notify child presenters
  notifyAutoreductionPaused();
}

void BatchPresenter::notifyAutoreductionPaused() {
  // Notify child presenters
  m_previewPresenter->notifyAutoreductionPaused();
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

void BatchPresenter::notifyAnyBatchReductionResumed() { m_runsPresenter->notifyAnyBatchReductionResumed(); }

void BatchPresenter::notifyAnyBatchReductionPaused() { m_runsPresenter->notifyAnyBatchReductionPaused(); }

void BatchPresenter::notifyAnyBatchAutoreductionResumed() { m_runsPresenter->notifyAnyBatchAutoreductionResumed(); }

void BatchPresenter::notifyAnyBatchAutoreductionPaused() { m_runsPresenter->notifyAnyBatchAutoreductionPaused(); }

void BatchPresenter::notifyBatchLoaded() { m_runsPresenter->notifyBatchLoaded(); }

void BatchPresenter::notifyRowContentChanged(Row &changedRow) { m_model->updateLookupIndex(changedRow); }

void BatchPresenter::notifyGroupNameChanged(Group &changedGroup) { m_model->updateLookupIndexesOfGroup(changedGroup); }

void BatchPresenter::notifyRunsTransferred() {
  m_model->updateLookupIndexesOfTable();
  m_runsPresenter->notifyRowModelChanged();
}

Mantid::Geometry::Instrument_const_sptr BatchPresenter::instrument() const { return m_mainPresenter->instrument(); }

std::string BatchPresenter::instrumentName() const { return m_mainPresenter->instrumentName(); }

void BatchPresenter::settingsChanged() {
  setBatchUnsaved();
  m_model->updateLookupIndexesOfTable();
  m_runsPresenter->settingsChanged();
}

/**
   Checks whether or not data is currently being processed in this batch
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isProcessing() const { return m_jobManager->isProcessing(); }

/**
   Checks whether or not autoprocessing is currently running in this batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isAutoreducing() const { return m_jobManager->isAutoreducing(); }

/**
   Checks whether or not processing is currently running in any batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isAnyBatchProcessing() const { return m_mainPresenter->isAnyBatchProcessing(); }

/**
   Checks whether or not autoprocessing is currently running in any batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being autoprocessed
   */
bool BatchPresenter::isAnyBatchAutoreducing() const { return m_mainPresenter->isAnyBatchAutoreducing(); }

bool BatchPresenter::isOverwriteBatchPrevented() const { return m_mainPresenter->isOverwriteBatchPrevented(this); }

bool BatchPresenter::discardChanges(std::string const &message) const {
  return m_mainPresenter->discardChanges(message);
}

/** Returns whether there are any unsaved changes in the current batch */
bool BatchPresenter::isBatchUnsaved() const { return m_unsavedBatchFlag || m_runsPresenter->hasUnsavedChanges(); }

void BatchPresenter::setBatchUnsaved() { m_unsavedBatchFlag = true; }

void BatchPresenter::notifyChangesSaved() {
  m_unsavedBatchFlag = false;
  m_runsPresenter->notifyChangesSaved();
}

void BatchPresenter::notifySetRoundPrecision(int &precision) { m_runsPresenter->setRoundPrecision(precision); }

void BatchPresenter::notifyResetRoundPrecision() { m_runsPresenter->resetRoundPrecision(); }

/** Get the percent of jobs that have been completed out of the current
    processing list
 */
int BatchPresenter::percentComplete() const { return m_jobManager->percentComplete(); }

std::unique_ptr<Mantid::API::IAlgorithmRuntimeProps> BatchPresenter::rowProcessingProperties() const {
  return m_jobManager->rowProcessingProperties();
}

void BatchPresenter::postDeleteHandle(const std::string &wsName) {
  auto const item = m_jobManager->notifyWorkspaceDeleted(wsName);
  m_runsPresenter->notifyRowModelChanged(item);
}

void BatchPresenter::renameHandle(const std::string &oldName, const std::string &newName) {
  auto const item = m_jobManager->notifyWorkspaceRenamed(oldName, newName);
  m_runsPresenter->notifyRowModelChanged(item);
}

void BatchPresenter::clearADSHandle() {
  m_jobManager->notifyAllWorkspacesDeleted();
  m_runsPresenter->notifyRowModelChanged();
}

void BatchPresenter::notifyPreviewApplyRequested() {
  auto const &previewRow = m_previewPresenter->getPreviewRow();
  m_experimentPresenter->notifyPreviewApplyRequested(previewRow);
}

std::map<ROIType, ProcessingInstructions> BatchPresenter::getMatchingProcessingInstructionsForPreviewRow() const {
  std::map<ROIType, ProcessingInstructions> roiMap;

  auto const &previewRow = m_previewPresenter->getPreviewRow();
  try {
    if (auto const lookupRow = m_model->findLookupRow(previewRow)) {
      if (auto const signalROI = lookupRow->processingInstructions()) {
        roiMap[ROIType::Signal] = signalROI.get();
      }
      if (auto const backgroundROI = lookupRow->backgroundProcessingInstructions()) {
        roiMap[ROIType::Background] = backgroundROI.get();
      }
      if (auto const transmissionROI = lookupRow->transmissionProcessingInstructions()) {
        roiMap[ROIType::Transmission] = transmissionROI.get();
      }
    }
  } catch (MultipleRowsFoundException const &) {
    // Do nothing, we will just return an empty map
  }

  return roiMap;
}

boost::optional<ProcessingInstructions> BatchPresenter::getMatchingROIDetectorIDsForPreviewRow() const {
  auto const &previewRow = m_previewPresenter->getPreviewRow();
  try {
    if (auto const lookupRow = m_model->findLookupRow(previewRow)) {
      return lookupRow->roiDetectorIDs();
    }
  } catch (MultipleRowsFoundException const &) {
    // Do nothing
  }
  return boost::none;
}
} // namespace MantidQt::CustomInterfaces::ISISReflectometry
