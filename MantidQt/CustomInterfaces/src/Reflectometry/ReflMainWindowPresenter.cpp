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

/** Returns global pre-processing options
* @return :: Global pre-processing options
*/
std::map<std::string, std::string>
ReflMainWindowPresenter::getPreprocessingOptions() {

  // Empty map at present
  // Options to 'CreateTransmissionWorkspaceAuto' are likely to be added
  // in the future
  return std::map<std::string, std::string>();
}

/** Returns global processing options
* @return :: Global processing options
*/
std::string ReflMainWindowPresenter::getProcessingOptions() {

  // Request global processing options to 'Settings' presenter
  return m_settingsPresenter->getProcessingOptions();
}

/** Returns global post-processing options
* @return :: Global post-processing options
*/
std::string ReflMainWindowPresenter::getPostprocessingOptions() {

  // Request global post-processing options to 'Settings' presenter
  return m_settingsPresenter->getPostprocessingOptions();
}
}
}