// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflBatchPresenter.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Experiment/IExperimentPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "GUI/Save/ISavePresenter.h"
#include "IReflBatchView.h"
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
ReflBatchPresenter::ReflBatchPresenter(
    IReflBatchView *view, std::unique_ptr<IRunsPresenter> runsPresenter,
    std::unique_ptr<IEventPresenter> eventPresenter,
    std::unique_ptr<IExperimentPresenter> experimentPresenter,
    std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
    std::unique_ptr<ISavePresenter> savePresenter)
    : m_view(view), m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(std::move(eventPresenter)),
      m_experimentPresenter(std::move(experimentPresenter)),
      m_instrumentPresenter(std::move(instrumentPresenter)),
      m_savePresenter(std::move(savePresenter)) {

  // Tell the tab presenters that this is going to be the main presenter
  m_runsPresenter->acceptMainPresenter(this);
}

bool ReflBatchPresenter::requestClose() const { return true; }

void ReflBatchPresenter::notifyInstrumentChanged(
    const std::string &instrumentName) const {
  setInstrumentName(instrumentName);
}

void ReflBatchPresenter::notifyReductionResumed() { reductionResumed(); }

void ReflBatchPresenter::notifyReductionPaused() { reductionPaused(); }

void ReflBatchPresenter::notifyReductionCompletedForGroup(
    GroupData const &group, std::string const &workspaceName) {
  reductionCompletedForGroup(group, workspaceName);
}

void ReflBatchPresenter::notifyReductionCompletedForRow(
    GroupData const &group, std::string const &workspaceName) {
  reductionCompletedForRow(group, workspaceName);
}

void ReflBatchPresenter::notifyAutoreductionResumed() {
  autoreductionResumed();
}

void ReflBatchPresenter::notifyAutoreductionPaused() { autoreductionPaused(); }

void ReflBatchPresenter::notifyAutoreductionCompleted() {
  autoreductionCompleted();
}

void ReflBatchPresenter::reductionResumed() {
  m_savePresenter->reductionResumed();
  m_eventPresenter->reductionResumed();
  m_experimentPresenter->reductionResumed();
  m_instrumentPresenter->reductionResumed();
}

void ReflBatchPresenter::reductionPaused() {
  m_savePresenter->reductionPaused();
  m_eventPresenter->reductionPaused();
  m_experimentPresenter->reductionPaused();
  m_instrumentPresenter->reductionPaused();
}

void ReflBatchPresenter::reductionCompletedForGroup(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->reductionCompletedForGroup(group, workspaceName);
}

void ReflBatchPresenter::reductionCompletedForRow(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->reductionCompletedForRow(group, workspaceName);
}

void ReflBatchPresenter::autoreductionResumed() {}

void ReflBatchPresenter::autoreductionPaused() {}

void ReflBatchPresenter::autoreductionCompleted() {}

void ReflBatchPresenter::settingsChanged() {
  m_runsPresenter->settingsChanged();
}

/** Returns default values specified for 'Transmission run(s)' for the
 * given angle
 *
 * @param angle :: the run angle to look up transmission runs for
 * @return :: Values passed for 'Transmission run(s)'
 */
OptionsQMap
ReflBatchPresenter::getOptionsForAngle(const double /*angle*/) const {
  return OptionsQMap(); // TODO m_settingsPresenter->getOptionsForAngle(angle);
}

/** Returns whether there are per-angle transmission runs specified
 * @return :: true if there are per-angle transmission runs
 * */
bool ReflBatchPresenter::hasPerAngleOptions() const {
  return false; // TODO m_settingsPresenter->hasPerAngleOptions();
}

/**
Tells the setting tab presenter what to set its current instrument name to.
* Also updates the config service, printing an information message
* @param instrumentName : The name of the instrument to be set
*/
void ReflBatchPresenter::setInstrumentName(
    const std::string &instrumentName) const {
  Mantid::Kernel::ConfigService::Instance().setString("default.instrument",
                                                      instrumentName);
  g_log.information() << "Instrument changed to " << instrumentName;
  m_runsPresenter->setInstrumentName(instrumentName);
  m_instrumentPresenter->setInstrumentName(instrumentName);
}

/**
Checks whether or not data is currently being processed in the Runs Tab
* @return : Bool on whether data is being processed
*/
bool ReflBatchPresenter::isProcessing() const {
  return m_runsPresenter->isProcessing();
}
} // namespace CustomInterfaces
} // namespace MantidQt
