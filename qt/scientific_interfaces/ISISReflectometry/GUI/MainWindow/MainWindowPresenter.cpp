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
  for (auto *batchView : m_view->batches())
    m_batchPresenters.emplace_back(m_batchPresenterFactory.make(batchView));
}

void MainWindowPresenter::notifyNewBatchRequested() {
  auto *newBatchView = m_view->newBatch();
  m_batchPresenters.emplace_back(m_batchPresenterFactory.make(newBatchView));
}

void MainWindowPresenter::notifyCloseBatchRequested(int batchIndex) {
  if (m_batchPresenters[batchIndex]->requestClose()) {
    m_batchPresenters.erase(m_batchPresenters.begin() + batchIndex);
    m_view->removeBatch(batchIndex);
  }
}

/**
   Used by the view to tell the presenter something has changed
*/
void MainWindowPresenter::notifyHelpPressed() { showHelp(); }

bool MainWindowPresenter::isProcessing() const {
  for (auto batchPresenter : m_batchPresenters) {
    if (batchPresenter->isProcessing())
      return true;
  }
  return false;
}

void MainWindowPresenter::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("ISIS Reflectometry"));
}
} // namespace CustomInterfaces
} // namespace MantidQt
