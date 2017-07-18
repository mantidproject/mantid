#include "MantidQtCustomInterfaces/Reflectometry/ReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflEventTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabPresenter.h"

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
    IReflSaveTabPresenter *savePresenter)
    : m_view(view), m_runsPresenter(runsPresenter),
      m_eventPresenter(eventPresenter), m_settingsPresenter(settingsPresenter),
      m_savePresenter(savePresenter), m_isProcessing(false) {

  // Tell the tab presenters that this is going to be the main presenter
  m_runsPresenter->acceptMainPresenter(this);
  m_savePresenter->acceptMainPresenter(this);
  // Settings tab does not need a main presenter

  // Trigger the setting of the current instrument name in settings tab
  m_runsPresenter->notify(IReflRunsTabPresenter::InstrumentChangedFlag);
}

/** Destructor
*/
ReflMainWindowPresenter::~ReflMainWindowPresenter() {}

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
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

/** Returns values passed for 'Transmission run(s)'
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*values
* @return :: Values passed for 'Transmission run(s)'
*/
std::string ReflMainWindowPresenter::getTransmissionRuns(int group) const {

  checkSettingsPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getTransmissionRuns(group, false);
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string ReflMainWindowPresenter::getTransmissionOptions(int group) const {

  checkSettingsPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getTransmissionOptions(group);
}

/** Returns global processing options
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global processing options
*/
std::string ReflMainWindowPresenter::getReductionOptions(int group) const {

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
