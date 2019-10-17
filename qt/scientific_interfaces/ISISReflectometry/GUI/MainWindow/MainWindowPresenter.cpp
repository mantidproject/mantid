// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MainWindowPresenter.h"
#include "GUI/Batch/IBatchPresenterFactory.h"
#include "GUI/Common/Decoder.h"
#include "GUI/Common/Encoder.h"
#include "GUI/Common/IMessageHandler.h"
#include "GUI/Runs/IRunsPresenter.h"
#include "IMainWindowView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ISlitCalculator.h"
#include "MantidQtWidgets/Common/QtJSONUtils.h"
#include "Reduction/Batch.h"

#include <QFileDialog>

namespace MantidQt {
namespace CustomInterfaces {
namespace ISISReflectometry {

using Mantid::API::AlgorithmManager;
using Mantid::API::MatrixWorkspace_sptr;
using MantidWidgets::ISlitCalculator;

// unnamed namespace
namespace {
Mantid::Kernel::Logger g_log("Reflectometry GUI");
}

/** Constructor
 * @param view :: [input] The view we are managing
 * @param messageHandler :: Interface to a class that displays messages to
 * the user
 * @param slitCalculator :: Interface to the Slit Calculator dialog
 * @param batchPresenterFactory :: [input] A factory to create the batches
 * we will manage
 */
MainWindowPresenter::MainWindowPresenter(
    IMainWindowView *view, IMessageHandler *messageHandler,
    std::unique_ptr<ISlitCalculator> slitCalculator,
    std::unique_ptr<IBatchPresenterFactory> batchPresenterFactory)
    : m_view(view), m_messageHandler(messageHandler), m_instrument(),
      m_slitCalculator(std::move(slitCalculator)),
      m_batchPresenterFactory(std::move(batchPresenterFactory)) {
  view->subscribe(this);
  for (auto *batchView : m_view->batches())
    addNewBatch(batchView);
}

MainWindowPresenter::~MainWindowPresenter() = default;

MainWindowPresenter::MainWindowPresenter(MainWindowPresenter &&) = default;

MainWindowPresenter &MainWindowPresenter::
operator=(MainWindowPresenter &&) = default;

void MainWindowPresenter::notifyNewBatchRequested() {
  auto *newBatchView = m_view->newBatch();
  addNewBatch(newBatchView);
}

void MainWindowPresenter::notifyCloseBatchRequested(int batchIndex) {
  if (m_batchPresenters[batchIndex]->isAutoreducing() ||
      m_batchPresenters[batchIndex]->isProcessing()) {
    m_messageHandler->giveUserCritical(
        "Cannot close batch while processing or autoprocessing is in progress",
        "Error");
    return;
  }

  if (m_batchPresenters[batchIndex]->requestClose()) {
    m_batchPresenters.erase(m_batchPresenters.begin() + batchIndex);
    m_view->removeBatch(batchIndex);
  }
}

void MainWindowPresenter::notifyShowOptionsRequested() {
  // TODO Show the options dialog when it is implemented
}

void MainWindowPresenter::notifyShowSlitCalculatorRequested() {
  m_slitCalculator->setCurrentInstrumentName(instrumentName());
  m_slitCalculator->processInstrumentHasBeenChanged();
  m_slitCalculator->show();
}

void MainWindowPresenter::notifyAnyBatchAutoreductionResumed() {
  for (const auto &batchPresenter : m_batchPresenters) {
    batchPresenter->notifyAnyBatchAutoreductionResumed();
  }
}

void MainWindowPresenter::notifyAnyBatchAutoreductionPaused() {
  for (const auto &batchPresenter : m_batchPresenters) {
    batchPresenter->notifyAnyBatchAutoreductionPaused();
  }
}

// Called on autoreduction normal reduction
void MainWindowPresenter::notifyAnyBatchReductionResumed() {
  for (const auto &batchPresenter : m_batchPresenters) {
    batchPresenter->notifyAnyBatchReductionResumed();
  }
  disableSaveAndLoadBatch();
}

// Called on autoreduction normal reduction
void MainWindowPresenter::notifyAnyBatchReductionPaused() {
  for (const auto &batchPresenter : m_batchPresenters) {
    batchPresenter->notifyAnyBatchReductionPaused();
  }
  enableSaveAndLoadBatch();
}

void MainWindowPresenter::notifyChangeInstrumentRequested(
    std::string const &instrumentName) {
  // Re-load instrument with the new name
  updateInstrument(instrumentName);
}

void MainWindowPresenter::notifyUpdateInstrumentRequested() {
  // An instrument should have been set up before any calls to this function.
  if (!instrument())
    throw std::runtime_error("Internal error: instrument has not been set");
  // Re-load instrument with the existing name.
  updateInstrument(instrumentName());
}

void MainWindowPresenter::notifyHelpPressed() { showHelp(); }

bool MainWindowPresenter::isAnyBatchProcessing() const {
  for (const auto &batchPresenter : m_batchPresenters) {
    if (batchPresenter->isProcessing())
      return true;
  }
  return false;
}

bool MainWindowPresenter::isAnyBatchAutoreducing() const {
  for (const auto &batchPresenter : m_batchPresenters) {
    if (batchPresenter->isAutoreducing())
      return true;
  }
  return false;
}

void MainWindowPresenter::addNewBatch(IBatchView *batchView) {
  // Remember the instrument name so we can re-set it (it will otherwise
  // get overridden by the instrument list default in the new batch)
  auto const instrument = instrumentName();
  m_batchPresenters.emplace_back(m_batchPresenterFactory->make(batchView));
  m_batchPresenters.back()->acceptMainPresenter(this);
  initNewBatch(m_batchPresenters.back().get(), instrument);
}

void MainWindowPresenter::initNewBatch(IBatchPresenter *batchPresenter,
                                       std::string const &instrument) {

  batchPresenter->initInstrumentList();
  batchPresenter->notifyInstrumentChanged(instrument);

  // starts in the paused state
  batchPresenter->notifyReductionPaused();

  // Ensure autoreduce button is enabled/disabled correctly for the new batch
  if (isAnyBatchAutoreducing())
    batchPresenter->notifyAnyBatchAutoreductionResumed();
  else
    batchPresenter->notifyAnyBatchAutoreductionPaused();
}

void MainWindowPresenter::showHelp() {
  MantidQt::API::HelpWindow::showCustomInterface(nullptr,
                                                 QString("ISIS Reflectometry"));
}

void MainWindowPresenter::notifySaveBatchRequested(int tabIndex) {
  auto filename = QFileDialog::getSaveFileName();
  if (filename == "")
    return;
  Encoder encoder;
  IBatchPresenter *batchPresenter = m_batchPresenters[tabIndex].get();
  auto map = encoder.encodeBatch(batchPresenter, m_view, false);
  MantidQt::API::saveJSONToFile(filename, map);
}

void MainWindowPresenter::notifyLoadBatchRequested(int tabIndex) {
  auto filename = QFileDialog::getOpenFileName();
  if (filename == "")
    return;
  auto map = MantidQt::API::loadJSONFromFile(filename);
  IBatchPresenter *batchPresenter = m_batchPresenters[tabIndex].get();
  Decoder decoder;
  decoder.decodeBatch(batchPresenter, m_view, map);
}

void MainWindowPresenter::disableSaveAndLoadBatch() {
  m_view->disableSaveAndLoadBatch();
}

void MainWindowPresenter::enableSaveAndLoadBatch() {
  m_view->enableSaveAndLoadBatch();
}

Mantid::Geometry::Instrument_const_sptr
MainWindowPresenter::instrument() const {
  return m_instrument;
}

std::string MainWindowPresenter::instrumentName() const {
  if (m_instrument)
    return m_instrument->getName();

  return std::string();
}

void MainWindowPresenter::updateInstrument(const std::string &instrumentName) {
  Mantid::Kernel::ConfigService::Instance().setString("default.instrument",
                                                      instrumentName);
  g_log.information() << "Instrument changed to " << instrumentName;

  // Load a workspace for this instrument so we can get the actual instrument
  auto loadAlg =
      AlgorithmManager::Instance().createUnmanaged("LoadEmptyInstrument");
  loadAlg->setChild(true);
  loadAlg->initialize();
  loadAlg->setProperty("InstrumentName", instrumentName);
  loadAlg->setProperty("OutputWorkspace",
                       "__Reflectometry_GUI_Empty_Instrument");
  loadAlg->execute();
  MatrixWorkspace_sptr instWorkspace = loadAlg->getProperty("OutputWorkspace");
  m_instrument = instWorkspace->getInstrument();

  // Notify child presenters
  for (auto &batchPresenter : m_batchPresenters)
    batchPresenter->notifyInstrumentChanged(instrumentName);

  // Notify the slit calculator
  m_slitCalculator->setCurrentInstrumentName(instrumentName);
  m_slitCalculator->processInstrumentHasBeenChanged();
}
} // namespace ISISReflectometry
} // namespace CustomInterfaces
} // namespace MantidQt
