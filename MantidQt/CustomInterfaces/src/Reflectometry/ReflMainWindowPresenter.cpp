#include "MantidQtCustomInterfaces/Reflectometry/ReflMainWindowPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflRunsTabPresenter.h"
#include "MantidQtCustomInterfaces/Reflectometry/IReflSettingsTabPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
* @param view :: [input] The view we are managing
* @param runsPresenter :: [input] A pointer to the 'Runs' tab presenter
* @param settingsPresenter :: [input] A pointer to the 'Settings' tab presenter
*/
ReflMainWindowPresenter::ReflMainWindowPresenter(
    IReflMainWindowView *view, IReflRunsTabPresenter *runsPresenter,
    IReflSettingsTabPresenter *settingsPresenter)
    : m_view(view), m_runsPresenter(runsPresenter),
      m_settingsPresenter(settingsPresenter) {

  // Tell the tab presenters that this is going to be the main presenter
  m_runsPresenter->acceptMainPresenter(this);
  m_settingsPresenter->acceptMainPresenter(this);
}

/** Destructor
*/
ReflMainWindowPresenter::~ReflMainWindowPresenter() {}

/** Returns global options for 'Plus' algorithm
* @return :: Global options for 'Plus' algorithm
*/
std::string ReflMainWindowPresenter::getPlusOptions() const {

  checkPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getPlusOptions();
}

/** Returns global options for 'CreateTransmissionWorkspaceAuto'
* @return :: Global options for 'CreateTransmissionWorkspaceAuto'
*/
std::string ReflMainWindowPresenter::getTransmissionOptions() const {

  checkPtrValid(m_settingsPresenter);

  return m_settingsPresenter->getTransmissionOptions();
}

/** Returns global processing options
* @return :: Global processing options
*/
std::string ReflMainWindowPresenter::getReductionOptions() const {

  checkPtrValid(m_settingsPresenter);

  // Request global processing options to 'Settings' presenter
  return m_settingsPresenter->getReductionOptions();
}

/** Returns global post-processing options
* @return :: Global post-processing options
*/
std::string ReflMainWindowPresenter::getStitchOptions() const {

  checkPtrValid(m_settingsPresenter);

  // Request global post-processing options to 'Settings' presenter
  return m_settingsPresenter->getStitchOptions();
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

std::string ReflMainWindowPresenter::getInstrument() const {

  return m_runsPresenter->getCurrentInstrument();
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
