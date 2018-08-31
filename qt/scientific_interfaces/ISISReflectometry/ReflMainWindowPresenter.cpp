#include "ReflMainWindowPresenter.h"
#include "IReflMainWindowView.h"
#include "IReflRunsTabPresenter.h"
#include "GUI/Event/IEventPresenter.h"
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
    IReflMainWindowView *view, ReflBatchPresenterFactory batchPresenterFactory)
    : m_view(view), m_batchPresenterFactory(std::move(batchPresenterFactory)) {
  view->subscribe(this);
  for (auto *batchView : m_view->batches())
    m_batchPresenters.emplace_back(m_batchPresenterFactory.make(batchView));
}

void ReflMainWindowPresenter::notifyNewBatchRequested() {
  auto *newBatchView = m_view->newBatch();
  m_batchPresenters.emplace_back(m_batchPresenterFactory.make(newBatchView));
}

void ReflMainWindowPresenter::notifyCloseBatchRequested(int batchIndex) {
  if (m_batchPresenters[batchIndex]->requestClose()) {
    m_batchPresenters.erase(m_batchPresenters.begin() + batchIndex);
    m_view->removeBatch(batchIndex);
  }
}

/**
Used by the view to tell the presenter something has changed
*/
void ReflMainWindowPresenter::notifyHelpPressed() { showHelp(); }

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
