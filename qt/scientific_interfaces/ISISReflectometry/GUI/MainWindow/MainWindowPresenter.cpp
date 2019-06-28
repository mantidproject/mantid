// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MainWindowPresenter.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "IMainWindowView.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "Reduction/Batch.h"

using namespace MantidQt::MantidWidgets::DataProcessor;

namespace MantidQt {
namespace CustomInterfaces {

/** Constructor
 * @param view :: [input] The view we are managing
 * @param batchPresenterFactory :: [input] A factory to create the batches
 * we will manage
 */
MainWindowPresenter::MainWindowPresenter(
    IMainWindowView *view, BatchPresenterFactory batchPresenterFactory)
    : m_view(view), m_batchPresenterFactory(std::move(batchPresenterFactory)) {
  view->subscribe(this);
  for (auto *batchView : m_view->batches())
    addNewBatch(batchView);
}

void MainWindowPresenter::notifyNewBatchRequested() {
  auto *newBatchView = m_view->newBatch();
  addNewBatch(newBatchView);
}

void MainWindowPresenter::notifyCloseBatchRequested(int batchIndex) {
  if (m_batchPresenters[batchIndex]->requestClose()) {
    m_batchPresenters.erase(m_batchPresenters.begin() + batchIndex);
    m_view->removeBatch(batchIndex);
  }
}

void MainWindowPresenter::notifyAutoreductionResumed() {
  for (auto batchPresenter : m_batchPresenters) {
    batchPresenter->anyBatchAutoreductionResumed();
  }
}

void MainWindowPresenter::notifyAutoreductionPaused() {
  for (auto batchPresenter : m_batchPresenters) {
    batchPresenter->anyBatchAutoreductionResumed();
  }
}

void MainWindowPresenter::notifyHelpPressed() { showHelp(); }

bool MainWindowPresenter::isAnyBatchProcessing() const {
  for (auto batchPresenter : m_batchPresenters) {
    if (batchPresenter->isProcessing())
      return true;
  }
  return false;
}

bool MainWindowPresenter::isAnyBatchAutoreducing() const {
  for (auto batchPresenter : m_batchPresenters) {
    if (batchPresenter->isAutoreducing())
      return true;
  }
  return false;
}

void MainWindowPresenter::addNewBatch(IBatchView *batchView) {
  m_batchPresenters.emplace_back(m_batchPresenterFactory.make(batchView));
  m_batchPresenters.back()->acceptMainPresenter(this);
  // Ensure autoreduce button is enabled/disabled correctly for the new batch
  if (isAnyBatchAutoreducing())
    m_batchPresenters.back()->anyBatchAutoreductionResumed();
  else
    m_batchPresenters.back()->anyBatchAutoreductionPaused();
}

void MainWindowPresenter::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("ISIS Reflectometry"));
}
} // namespace CustomInterfaces
} // namespace MantidQt
