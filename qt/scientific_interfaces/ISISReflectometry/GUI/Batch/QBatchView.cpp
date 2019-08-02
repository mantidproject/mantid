// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QBatchView.h"
#include "GUI/Event/QEventView.h"
#include "GUI/Runs/QRunsView.h"
#include "GUI/Save/QSaveView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include <QMessageBox>
#include <QMetaType>

namespace MantidQt {
namespace CustomInterfaces {

using API::BatchAlgorithmRunner;
using Mantid::API::IAlgorithm_sptr;

QBatchView::QBatchView(QWidget *parent)
    : QWidget(parent), m_batchAlgoRunner(this) {
  qRegisterMetaType<API::IConfiguredAlgorithm_sptr>(
      "MantidQt::API::IConfiguredAlgorithm_sptr");
  initLayout();
  m_batchAlgoRunner.stopOnFailure(false);
  connectBatchAlgoRunnerSlots();
}

void QBatchView::subscribe(BatchViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void QBatchView::initLayout() {
  m_ui.setupUi(this);

  m_runs = createRunsTab();
  m_ui.batchTabs->addTab(m_runs.get(), "Runs");

  m_eventHandling = createEventTab();
  m_ui.batchTabs->addTab(m_eventHandling.get(), "Event Handling");

  m_experiment = std::make_unique<QExperimentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_experiment.get(), "Experiment Settings");

  m_instrument = std::make_unique<QInstrumentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_instrument.get(), "Instrument Settings");

  m_save = createSaveTab();
  m_ui.batchTabs->addTab(m_save.get(), "Save ASCII");
}

IExperimentView *QBatchView::experiment() const { return m_experiment.get(); }

IInstrumentView *QBatchView::instrument() const { return m_instrument.get(); }

IRunsView *QBatchView::runs() const { return m_runs.get(); }

IEventView *QBatchView::eventHandling() const { return m_eventHandling.get(); }

ISaveView *QBatchView::save() const { return m_save.get(); }

void QBatchView::clearAlgorithmQueue() { m_batchAlgoRunner.clearQueue(); }

void QBatchView::setAlgorithmQueue(
    std::deque<API::IConfiguredAlgorithm_sptr> algorithms) {
  m_batchAlgoRunner.setQueue(algorithms);
}

void QBatchView::connectBatchAlgoRunnerSlots() {
  connect(&m_batchAlgoRunner, SIGNAL(batchComplete(bool)), this,
          SLOT(onBatchComplete(bool)));
  connect(&m_batchAlgoRunner, SIGNAL(batchCancelled()), this,
          SLOT(onBatchCancelled()));
  connect(&m_batchAlgoRunner,
          SIGNAL(algorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr)),
          this,
          SLOT(onAlgorithmStarted(MantidQt::API::IConfiguredAlgorithm_sptr)));
  connect(&m_batchAlgoRunner,
          SIGNAL(algorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr)),
          this,
          SLOT(onAlgorithmComplete(MantidQt::API::IConfiguredAlgorithm_sptr)));
  connect(&m_batchAlgoRunner,
          SIGNAL(algorithmError(MantidQt::API::IConfiguredAlgorithm_sptr,
                                std::string)),
          this,
          SLOT(onAlgorithmError(MantidQt::API::IConfiguredAlgorithm_sptr,
                                std::string)));
}

void QBatchView::executeAlgorithmQueue() {
  m_batchAlgoRunner.executeBatchAsync();
}

void QBatchView::cancelAlgorithmQueue() { m_batchAlgoRunner.cancelBatch(); }

void QBatchView::onBatchComplete(bool error) {
  m_notifyee->notifyBatchComplete(error);
}

void QBatchView::onBatchCancelled() { m_notifyee->notifyBatchCancelled(); }

void QBatchView::onAlgorithmStarted(API::IConfiguredAlgorithm_sptr algorithm) {
  m_notifyee->notifyAlgorithmStarted(algorithm);
}

void QBatchView::onAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) {
  m_notifyee->notifyAlgorithmComplete(algorithm);
}

void QBatchView::onAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm,
                                  std::string message) {
  m_notifyee->notifyAlgorithmError(algorithm, message);
}

std::unique_ptr<QRunsView> QBatchView::createRunsTab() {
  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  return std::make_unique<QRunsView>(this, RunsTableViewFactory(instruments));
}

std::unique_ptr<QEventView> QBatchView::createEventTab() {
  return std::make_unique<QEventView>(this);
}

IAlgorithm_sptr QBatchView::createReductionAlg() {
  return Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryReductionOneAuto");
}

std::unique_ptr<QSaveView> QBatchView::createSaveTab() {
  return std::make_unique<QSaveView>(this);
}
} // namespace CustomInterfaces
} // namespace MantidQt
