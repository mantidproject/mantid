#include "MantidQtCustomInterfaces/Reflectometry/ReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSaveTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: [input] The view we are managing
* @param runsPresenter :: [input] A pointer to the 'Runs' tab presenter
* @param settingsPresenter :: [input] A pointer to the 'Settings' tab presenter
* @param savePresenter :: [input] A pointer to the 'Save ASCII' tab presenter
*/
ReflMainWindowPresenter::ReflMainWindowPresenter(
    IReflMainWindowView *view, IReflRunsTabPresenter *runsPresenter,
    IReflSettingsTabPresenter *settingsPresenter,
    IReflSaveTabPresenter *savePresenter)
    : m_view(view), m_runsPresenter(runsPresenter),
      m_settingsPresenter(settingsPresenter), m_savePresenter(savePresenter) {

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

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string ReflMainWindowPresenter::getTransmissionOptions(int group) const {

  checkPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getTransmissionOptions(group);
}

/** Returns global processing options
*
* @param group :: Index of the group in 'Settings' tab from which to get the
*options
* @return :: Global processing options
*/
std::string ReflMainWindowPresenter::getReductionOptions(int group) const {

  checkPtrValid(m_settingsPresenter);

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

  checkPtrValid(m_settingsPresenter);

  // Request global post-processing options to 'Settings' presenter
  return m_settingsPresenter->getStitchOptions(group);
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
Tells the view to show a warning dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void ReflMainWindowPresenter::giveUserWarning(const std::string &prompt,
                                              const std::string &title) {

  m_view->giveUserWarning(prompt, title);
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
Tells the view to ask the user a Yes/No question
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@returns a boolean true if Yes, false if No
*/
bool ReflMainWindowPresenter::askUserYesNo(const std::string &prompt,
                                           const std::string &title) {

  return m_view->askUserYesNo(prompt, title);
}

/**
Tells the view to ask the user to enter a string.
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@param defaultValue : The default value entered.
@returns The user's string if submitted, or an empty string
*/
std::string
ReflMainWindowPresenter::askUserString(const std::string &prompt,
                                       const std::string &title,
                                       const std::string &defaultValue) {

  return m_view->askUserString(prompt, title, defaultValue);
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

/** Checks for null pointer
* @param pointer :: The pointer
*/
void ReflMainWindowPresenter::checkPtrValid(
    IReflSettingsTabPresenter *pointer) const {
  if (pointer == nullptr)
    throw std::invalid_argument("Could not read settings");
}
}
}
