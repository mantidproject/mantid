// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchPresenter.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IBatchView.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"

using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");
}

/** Constructor
 * @param view :: [input] The view we are managing
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
    : m_view(view),
      m_jobRunner(std::move(model), m_view->batchAlgorithmRunner()),
      m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(std::move(eventPresenter)),
      m_experimentPresenter(std::move(experimentPresenter)),
      m_instrumentPresenter(std::move(instrumentPresenter)),
      m_savePresenter(std::move(savePresenter)) {

  m_view->subscribe(this);

  // Tell the tab presenters that this is going to be the main presenter
  m_savePresenter->acceptMainPresenter(this);
  m_eventPresenter->acceptMainPresenter(this);
  m_experimentPresenter->acceptMainPresenter(this);
  m_instrumentPresenter->acceptMainPresenter(this);
  m_runsPresenter->acceptMainPresenter(this);
}

bool BatchPresenter::requestClose() const { return true; }

void BatchPresenter::notifyInstrumentChanged(
    const std::string &instrumentName) {
  instrumentChanged(instrumentName);
}

void BatchPresenter::notifySettingsChanged() { settingsChanged(); }

void BatchPresenter::notifyReductionResumed() { reductionResumed(); }

void BatchPresenter::notifyReductionPaused() { reductionPaused(); }

void BatchPresenter::notifyReductionCompletedForGroup(
    GroupData const &group, std::string const &workspaceName) {
  reductionCompletedForGroup(group, workspaceName);
}

void BatchPresenter::notifyReductionCompletedForRow(
    GroupData const &group, std::string const &workspaceName) {
  reductionCompletedForRow(group, workspaceName);
}

void BatchPresenter::notifyAutoreductionResumed() { autoreductionResumed(); }

void BatchPresenter::notifyAutoreductionPaused() { autoreductionPaused(); }

void BatchPresenter::notifyAutoreductionCompleted() {
  autoreductionCompleted();
}

void BatchPresenter::notifyBatchFinished(bool error) {
  UNUSED_ARG(error);
  reductionPaused();
  m_runsPresenter->notifyRowStateChanged();
}

void BatchPresenter::notifyBatchCancelled() {
  UNUSED_ARG(error);
  reductionPaused();
  m_runsPresenter->notifyRowStateChanged();
}

void BatchPresenter::notifyAlgorithmFinished() {
  m_runsPresenter->notifyRowStateChanged();
}

void BatchPresenter::notifyAlgorithmError(std::string const &message) {
  UNUSED_ARG(message);
  m_runsPresenter->notifyRowStateChanged();
}

void BatchPresenter::reductionResumed() {
  m_jobRunner.resumeReduction();
  m_view->executeBatchAlgorithmRunner();

  // Notify child presenters
  m_savePresenter->reductionResumed();
  m_eventPresenter->reductionResumed();
  m_experimentPresenter->reductionResumed();
  m_instrumentPresenter->reductionResumed();
  m_runsPresenter->reductionResumed();
}

void BatchPresenter::reductionPaused() {
  m_jobRunner.pauseReduction();
  // Notify child presenters
  m_savePresenter->reductionPaused();
  m_eventPresenter->reductionPaused();
  m_experimentPresenter->reductionPaused();
  m_instrumentPresenter->reductionPaused();
  m_runsPresenter->reductionPaused();
  // Also stop autoreduction
  autoreductionPaused();
}

void BatchPresenter::reductionCompletedForGroup(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->reductionCompletedForGroup(group, workspaceName);
}

void BatchPresenter::reductionCompletedForRow(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->reductionCompletedForRow(group, workspaceName);
}

void BatchPresenter::autoreductionResumed() {
  m_jobRunner.resumeAutoreduction();
  // Notify child presenters
  m_savePresenter->autoreductionResumed();
  m_eventPresenter->autoreductionResumed();
  m_experimentPresenter->autoreductionResumed();
  m_instrumentPresenter->autoreductionResumed();
  m_runsPresenter->autoreductionResumed();
}

void BatchPresenter::autoreductionPaused() {
  m_jobRunner.pauseAutoreduction();
  // Notify child presenters
  m_savePresenter->autoreductionPaused();
  m_eventPresenter->autoreductionPaused();
  m_experimentPresenter->autoreductionPaused();
  m_instrumentPresenter->autoreductionPaused();
  m_runsPresenter->autoreductionPaused();
}

void BatchPresenter::autoreductionCompleted() {}

void BatchPresenter::instrumentChanged(const std::string &instrumentName) {
  Mantid::Kernel::ConfigService::Instance().setString("default.instrument",
                                                      instrumentName);
  g_log.information() << "Instrument changed to " << instrumentName;
  m_runsPresenter->instrumentChanged(instrumentName);
  m_instrumentPresenter->instrumentChanged(instrumentName);
}

void BatchPresenter::settingsChanged() { m_runsPresenter->settingsChanged(); }

/** Returns default values specified for 'Transmission run(s)' for the
 * given angle
 *
 * @param angle :: the run angle to look up transmission runs for
 * @return :: Values passed for 'Transmission run(s)'
 */
OptionsQMap BatchPresenter::getOptionsForAngle(const double /*angle*/) const {
  return OptionsQMap(); // TODO m_settingsPresenter->getOptionsForAngle(angle);
}

/** Returns whether there are per-angle transmission runs specified
 * @return :: true if there are per-angle transmission runs
 * */
bool BatchPresenter::hasPerAngleOptions() const {
  return false; // TODO m_settingsPresenter->hasPerAngleOptions();
}

/**
   Checks whether or not data is currently being processed in this batch
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isProcessing() const { return m_jobRunner.isProcessing(); }

/**
   Checks whether or not autoprocessing is currently running in this batch
   * i.e. whether we are polling for new runs
   * @return : Bool on whether data is being processed
   */
bool BatchPresenter::isAutoreducing() const {
  return m_jobRunner.isAutoreducing();
}
} // namespace CustomInterfaces
} // namespace MantidQt
