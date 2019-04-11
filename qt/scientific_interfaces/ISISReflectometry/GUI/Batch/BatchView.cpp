// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchView.h"
#include "../MainWindow/MainWindowView.h"
#include "GUI/Event/EventView.h"
#include "GUI/Runs/RunsView.h"
#include "GUI/Save/SaveView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidAPI/IAlgorithm.h"
#include "MantidKernel/make_unique.h"
#include "MantidQtWidgets/Common/BatchAlgorithmRunner.h"

#include <QMessageBox>
#include <QMetaType>

namespace MantidQt {
namespace CustomInterfaces {

using API::BatchAlgorithmRunner;
using Mantid::API::IAlgorithm_sptr;

BatchView::BatchView(QWidget *parent)
    : QWidget(parent), m_batchAlgoRunner(this) {
  qRegisterMetaType<API::IConfiguredAlgorithm_sptr>(
      "MantidQt::API::IConfiguredAlgorithm_sptr");
  initLayout();
  m_batchAlgoRunner.stopOnFailure(false);
}

void BatchView::subscribe(BatchViewSubscriber *notifyee) {
  m_notifyee = notifyee;
}

void BatchView::initLayout() {
  m_ui.setupUi(this);

  m_runs = createRunsTab();
  m_ui.batchTabs->addTab(m_runs.get(), "Runs");

  m_eventHandling = createEventTab();
  m_ui.batchTabs->addTab(m_eventHandling.get(), "Event Handling");

  m_experiment =
      Mantid::Kernel::make_unique<ExperimentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_experiment.get(), "Experiment Settings");

  m_instrument =
      Mantid::Kernel::make_unique<InstrumentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_instrument.get(), "Instrument Settings");

  m_save = createSaveTab();
  m_ui.batchTabs->addTab(m_save.get(), "Save ASCII");
}

IExperimentView *BatchView::experiment() const { return m_experiment.get(); }

IInstrumentView *BatchView::instrument() const { return m_instrument.get(); }

IRunsView *BatchView::runs() const { return m_runs.get(); }

IEventView *BatchView::eventHandling() const { return m_eventHandling.get(); }

ISaveView *BatchView::save() const { return m_save.get(); }

void BatchView::clearAlgorithmQueue() { m_batchAlgoRunner.clearQueue(); }

void BatchView::setAlgorithmQueue(
    std::deque<API::IConfiguredAlgorithm_sptr> algorithms) {
  m_batchAlgoRunner.setQueue(algorithms);
}

void BatchView::executeAlgorithmQueue() {
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
  m_batchAlgoRunner.executeBatchAsync();
}

void BatchView::cancelAlgorithmQueue() { m_batchAlgoRunner.cancelBatch(); }

void BatchView::onBatchComplete(bool error) {
  m_notifyee->notifyBatchComplete(error);
}

void BatchView::onBatchCancelled() { m_notifyee->notifyBatchCancelled(); }

void BatchView::onAlgorithmStarted(API::IConfiguredAlgorithm_sptr algorithm) {
  m_notifyee->notifyAlgorithmStarted(algorithm);
}

void BatchView::onAlgorithmComplete(API::IConfiguredAlgorithm_sptr algorithm) {
  m_notifyee->notifyAlgorithmComplete(algorithm);
}

void BatchView::onAlgorithmError(API::IConfiguredAlgorithm_sptr algorithm,
                                 std::string message) {
  m_notifyee->notifyAlgorithmError(algorithm, message);
}

std::unique_ptr<RunsView> BatchView::createRunsTab() {
  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  return Mantid::Kernel::make_unique<RunsView>(
      this, RunsTableViewFactory(instruments));
}

std::unique_ptr<EventView> BatchView::createEventTab() {
  return Mantid::Kernel::make_unique<EventView>(this);
}

IAlgorithm_sptr BatchView::createReductionAlg() {
  return Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryReductionOneAuto");
}

std::unique_ptr<SaveView> BatchView::createSaveTab() {
  return Mantid::Kernel::make_unique<SaveView>(this);
}
} // namespace CustomInterfaces
} // namespace MantidQt
