#include "ReflBatchPresenter.h"
#include "GUI/Event/IEventPresenter.h"
#include "GUI/Instrument/IInstrumentPresenter.h"
#include "IReflBatchView.h"
#include "IReflRunsTabPresenter.h"
#include "IReflSaveTabPresenter.h"
#include "MantidQtWidgets/Common/HelpWindow.h"

using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: [input] The view we are managing
* @param runsPresenter :: [input] A pointer to the 'Runs' tab presenter
* @param eventPresenter :: [input] A pointer to the 'Event Handling' tab
* presenter
* @param instrumentPresenter :: [input] A pointer to the 'Instrument' tab
* presenter
* @param savePresenter :: [input] A pointer to the 'Save ASCII' tab presenter
*/
ReflBatchPresenter::ReflBatchPresenter(
    IReflBatchView *view, std::unique_ptr<IReflRunsTabPresenter> runsPresenter,
    std::unique_ptr<IEventPresenter> eventPresenter,
    std::unique_ptr<IExperimentPresenter> experimentPresenter,
    std::unique_ptr<IInstrumentPresenter> instrumentPresenter,
    std::unique_ptr<IReflSaveTabPresenter> savePresenter)
    : m_view(view), m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(std::move(eventPresenter)),
      m_experimentPresenter(std::move(experimentPresenter)),
      m_instrumentPresenter(std::move(instrumentPresenter)),
      m_savePresenter(std::move(savePresenter)) {

  // Tell the tab presenters that this is going to be the main presenter
  m_runsPresenter->acceptMainPresenter(this);
  m_savePresenter->acceptMainPresenter(this);
  m_eventPresenter->acceptMainPresenter(this);

  // Trigger the setting of the current instrument name in settings tab
  m_runsPresenter->notify(IReflRunsTabPresenter::InstrumentChangedFlag);
}

bool ReflBatchPresenter::requestClose() const { return true; }

void ReflBatchPresenter::completedGroupReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->completedGroupReductionSuccessfully(group, workspaceName);
}

void ReflBatchPresenter::completedRowReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->completedRowReductionSuccessfully(group, workspaceName);
}

void ReflBatchPresenter::notifyReductionPaused() {
  m_savePresenter->onAnyReductionPaused();
  m_eventPresenter->onReductionPaused();
  // m_instrumentPresenter->onReductionPaused();
}

void ReflBatchPresenter::notifyReductionResumed() {
  m_savePresenter->onAnyReductionResumed();
  m_eventPresenter->onReductionResumed();
  // m_instrumentPresenter->onReductionResumed();
}

void ReflBatchPresenter::settingsChanged() {
  m_runsPresenter->settingsChanged();
}

/** Returns default values specified for 'Transmission run(s)' for the
* given angle
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*values
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
Tells the setting tab presenter what to set its current instrument name to
* @param instName : The name of the instrument to be set
*/
void ReflBatchPresenter::setInstrumentName(
    const std::string & /*instName*/) const {
  return; // TODO m_settingsPresenter->setInstrumentName(instName);
}

/**
Checks whether or not data is currently being processed in the Runs Tab
* @return : Bool on whether data is being processed
*/
bool ReflBatchPresenter::isProcessing() const {
  return m_runsPresenter->isProcessing();
}
}
}
