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
ReflMainWindowPresenter::ReflMainWindowPresenter(IReflMainWindowView *view)
    : m_view(view) {
}

/**
Used by the view to tell the presenter something has changed
*/
void ReflMainWindowPresenter::notify(IReflMainWindowPresenter::Flag flag) {
  switch (flag) {
  case Flag::HelpPressed:
    showHelp();
    break;
  }
  // Not having a 'default' case is deliberate. gcc issues a warning if there's
  // a flag we aren't handling.
}

bool ReflMainWindowPresenter::isProcessing() const {
  // TODO Implement this once you have ownership of child presenters.
  return false;
}

void ReflMainWindowPresenter::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("ISIS Reflectometry"));
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

}
}
