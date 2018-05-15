#include "ReflMainWindowPresenter.h"
#include "IReflMainWindowView.h"
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
ReflMainWindowPresenter::ReflMainWindowPresenter(
    IReflMainWindowView *view, IReflRunsTabPresenter *runsPresenter,
    IReflEventTabPresenter *eventPresenter,
    IReflSettingsTabPresenter *settingsPresenter,
    std::unique_ptr<IReflSaveTabPresenter> savePresenter)
    : m_view(view), m_runsPresenter(runsPresenter),
      m_eventPresenter(eventPresenter), m_settingsPresenter(settingsPresenter),
      m_savePresenter(std::move(savePresenter)), m_isProcessing(false) {

  // Tell the tab presenters that this is going to be the main presenter
  m_runsPresenter->acceptMainPresenter(this);
  m_savePresenter->acceptMainPresenter(this);
  m_settingsPresenter->acceptMainPresenter(this);

  // Trigger the setting of the current instrument name in settings tab
  m_runsPresenter->notify(IReflRunsTabPresenter::InstrumentChangedFlag);
}

/** Destructor
*/
ReflMainWindowPresenter::~ReflMainWindowPresenter() {}

void ReflMainWindowPresenter::completedGroupReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->completedGroupReductionSuccessfully(group, workspaceName);
}

void ReflMainWindowPresenter::completedRowReductionSuccessfully(
    GroupData const &group, std::string const &workspaceName) {
  m_savePresenter->completedRowReductionSuccessfully(group, workspaceName);
}

void ReflMainWindowPresenter::notifyReductionPaused(int group) {
  m_isProcessing = false;
  m_savePresenter->onAnyReductionPaused();
  m_settingsPresenter->onReductionPaused(group);
  m_eventPresenter->onReductionPaused(group);
}

void ReflMainWindowPresenter::notifyReductionResumed(int group) {
  m_isProcessing = true;
  m_savePresenter->onAnyReductionResumed();
  m_settingsPresenter->onReductionResumed(group);
  m_eventPresenter->onReductionResumed(group);
}

/**
Used by the view to tell the presenter something has changed
*/
void ReflMainWindowPresenter::notify(IReflMainWindowPresenter::Flag flag) {

  switch (flag) {
  case Flag::ConfirmReductionPausedFlag:
    m_isProcessing = false;
    break;
  case Flag::ConfirmReductionResumedFlag:
    m_isProcessing = true;
    break;
  case Flag::HelpPressed:
    showHelp();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

void ReflMainWindowPresenter::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("ISIS Reflectometry"));
}

void ReflMainWindowPresenter::settingsChanged(int group) {
  m_runsPresenter->settingsChanged(group);
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
OptionsQMap ReflMainWindowPresenter::getTransmissionOptions(int group) const {

  checkSettingsPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getTransmissionOptions(group);
}

/** Returns global processing options
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global processing options
*/
OptionsQMap ReflMainWindowPresenter::getReductionOptions(int group) const {

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
std::string ReflMainWindowPresenter::getStitchOptions(int group) const {

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
std::string ReflMainWindowPresenter::getTimeSlicingValues(int group) const {

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
std::string ReflMainWindowPresenter::getTimeSlicingType(int group) const {

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
ReflMainWindowPresenter::getOptionsForAngle(int group,
                                            const double angle) const {

  checkSettingsPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getOptionsForAngle(group, angle);
}

/** Returns whether there are per-angle transmission runs specified
 * @return :: true if there are per-angle transmission runs
 * */
bool ReflMainWindowPresenter::hasPerAngleOptions(int group) const {
  checkSettingsPtrValid(m_settingsPresenter);
  return m_settingsPresenter->hasPerAngleOptions(group);
}

/**
Tells the view to show an critical error dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void ReflMainWindowPresenter::giveUserCritical(const std::string &prompt,
                                               const std::string &title) {

  m_view->giveUserCritical(prompt, title);
}

/**
Tells the view to show an information dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void ReflMainWindowPresenter::giveUserInfo(const std::string &prompt,
                                           const std::string &title) {

  m_view->giveUserInfo(prompt, title);
}

/**
Tells the view to show the user the dialog for an algorithm
* @param pythonCode : [input] The algorithm as python code
* @return : Result of the execution
*/
std::string
ReflMainWindowPresenter::runPythonAlgorithm(const std::string &pythonCode) {

  return m_view->runPythonAlgorithm(pythonCode);
}

/**
Tells the setting tab presenter what to set its current instrument name to
* @param instName : The name of the instrument to be set
*/
void ReflMainWindowPresenter::setInstrumentName(
    const std::string &instName) const {

  m_settingsPresenter->setInstrumentName(instName);
}

/**
Checks whether or not data is currently being processed in the Runs Tab
* @return : Bool on whether data is being processed
*/
bool ReflMainWindowPresenter::checkIfProcessing() const {

  return m_isProcessing;
}

/** Checks for Settings Tab null pointer
* @param pointer :: The pointer
*/
void ReflMainWindowPresenter::checkSettingsPtrValid(
    IReflSettingsTabPresenter *pointer) const {
  if (pointer == nullptr)
    throw std::invalid_argument("Could not read settings");
}

/** Checks for Event Handling Tab null pointer
* @param pointer :: The pointer
*/
void ReflMainWindowPresenter::checkEventPtrValid(
    IReflEventTabPresenter *pointer) const {
  if (pointer == nullptr)
    throw std::invalid_argument("Could not read event handling");
}
}
}
