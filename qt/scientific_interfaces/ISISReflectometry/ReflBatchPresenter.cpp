#include "ReflBatchPresenter.h"
#include "IReflBatchView.h"
#include "IReflRunsTabPresenter.h"
#include "IReflEventTabPresenter.h"
#include "IReflSettingsTabPresenter.h"
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
* @param settingsPresenter :: [input] A pointer to the 'Settings' tab presenter
* @param savePresenter :: [input] A pointer to the 'Save ASCII' tab presenter
*/
ReflBatchPresenter::ReflBatchPresenter(
    IReflBatchView *view,
    std::unique_ptr<IReflRunsTabPresenter> runsPresenter,
    IReflEventTabPresenter *eventPresenter,
    IReflSettingsTabPresenter *settingsPresenter,
    std::unique_ptr<IReflSaveTabPresenter> savePresenter)
    : m_view(view), m_runsPresenter(std::move(runsPresenter)),
      m_eventPresenter(eventPresenter), m_settingsPresenter(settingsPresenter),
      m_savePresenter(std::move(savePresenter)) {

  // Tell the tab presenters that this is going to be the main presenter
  m_runsPresenter->acceptMainPresenter(this);
  m_savePresenter->acceptMainPresenter(this);
  m_settingsPresenter->acceptMainPresenter(this);
  m_eventPresenter->acceptMainPresenter(this);

  // Trigger the setting of the current instrument name in settings tab
  m_runsPresenter->notify(IReflRunsTabPresenter::InstrumentChangedFlag);
}

/** Destructor
*/
ReflBatchPresenter::~ReflBatchPresenter() {}

void ReflBatchPresenter::completedGroupReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->completedGroupReductionSuccessfully(group, workspaceName);
}

void ReflBatchPresenter::completedRowReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->completedRowReductionSuccessfully(group, workspaceName);
}

void ReflBatchPresenter::notifyReductionPaused(int group) {
  m_savePresenter->onAnyReductionPaused();
  m_settingsPresenter->onReductionPaused(group);
  m_eventPresenter->onReductionPaused(group);
}

void ReflBatchPresenter::notifyReductionResumed(int group) {
  m_savePresenter->onAnyReductionResumed();
  m_settingsPresenter->onReductionResumed(group);
  m_eventPresenter->onReductionResumed(group);
}

void ReflBatchPresenter::settingsChanged(int group) {
  m_runsPresenter->settingsChanged(group);
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
OptionsQMap ReflBatchPresenter::getTransmissionOptions(int group) const {

  checkSettingsPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getTransmissionOptions(group);
}

/** Returns global processing options
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global processing options
*/
OptionsQMap ReflBatchPresenter::getReductionOptions(int group) const {

  checkSettingsPtrValid(m_settingsPresenter);

  // Request global processing options to 'Settings' presenter
  return m_settingsPresenter->getReductionOptions(group);
}

/** Returns global post-processing options
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global post-processing options
*/
std::string ReflBatchPresenter::getStitchOptions(int group) const {

  checkSettingsPtrValid(m_settingsPresenter);

  // Request global post-processing options to 'Settings' presenter
  return m_settingsPresenter->getStitchOptions(group);
}

/** Returns time-slicing values
*
* @param group :: Index of the group in 'Event Handling' tab from which to get
*the values
* @return :: Time-slicing values
*/
std::string ReflBatchPresenter::getTimeSlicingValues(int group) const {

  checkEventPtrValid(m_eventPresenter);

  // Request global time-slicing values to 'Event Handling' presenter
  return m_eventPresenter->getTimeSlicingValues(group);
}

/** Returns time-slicing type
*
* @param group :: Index of the group in 'Event Handling' tab from which to get
*the type
* @return :: Time-slicing type
*/
std::string ReflBatchPresenter::getTimeSlicingType(int group) const {

  checkEventPtrValid(m_eventPresenter);

  // Request time-slicing type to 'Event Handling' presenter
  return m_eventPresenter->getTimeSlicingType(group);
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
ReflBatchPresenter::getOptionsForAngle(int group,
                                            const double angle) const {

  checkSettingsPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getOptionsForAngle(group, angle);
}

/** Returns whether there are per-angle transmission runs specified
 * @return :: true if there are per-angle transmission runs
 * */
bool ReflBatchPresenter::hasPerAngleOptions(int group) const {
  checkSettingsPtrValid(m_settingsPresenter);
  return m_settingsPresenter->hasPerAngleOptions(group);
}

/**
Tells the setting tab presenter what to set its current instrument name to
* @param instName : The name of the instrument to be set
*/
void ReflBatchPresenter::setInstrumentName(
    const std::string &instName) const {

  m_settingsPresenter->setInstrumentName(instName);
}

/**
Checks whether or not data is currently being processed in the Runs Tab
* @return : Bool on whether data is being processed
*/
bool ReflBatchPresenter::isProcessing() const {
  return m_runsPresenter->isProcessing();
}

/**
Checks whether or not data is currently being processed in the Runs Tab
for a specific group
* @return : Bool on whether data is being processed
*/
bool ReflBatchPresenter::isProcessing(int group) const {
  return m_runsPresenter->isProcessing(group);
}

/** Checks for Settings Tab null pointer
* @param pointer :: The pointer
*/
void ReflBatchPresenter::checkSettingsPtrValid(
    IReflSettingsTabPresenter *pointer) const {
  if (pointer == nullptr)
    throw std::invalid_argument("Could not read settings");
}

/** Checks for Event Handling Tab null pointer
* @param pointer :: The pointer
*/
void ReflBatchPresenter::checkEventPtrValid(
    IReflEventTabPresenter *pointer) const {
  if (pointer == nullptr)
    throw std::invalid_argument("Could not read event handling");
}
}
}
