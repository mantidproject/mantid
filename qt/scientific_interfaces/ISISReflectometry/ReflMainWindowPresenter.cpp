// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "ReflMainWindowPresenter.h"
#include "IReflMainWindowView.h"
#include "IReflRunsTabPresenter.h"
#include "MantidQtWidgets/Common/HelpWindow.h"

using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param view :: [input] The view we are managing
 * @param batchPresenterFactory :: [input] A factory to create the batches
 * we will manage
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
} // namespace CustomInterfaces
} // namespace MantidQt
