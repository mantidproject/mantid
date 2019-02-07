// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "BatchView.h"
#include "GUI/Event/EventView.h"
#include "GUI/Runs/RunsView.h"
#include "GUI/Save/SaveView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/make_unique.h"

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {

BatchView::BatchView(QWidget *parent) : QWidget(parent) { initLayout(); }

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

std::unique_ptr<RunsView> BatchView::createRunsTab() {
  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  return Mantid::Kernel::make_unique<RunsView>(
      this, RunsTableViewFactory(instruments));
}

std::unique_ptr<EventView> BatchView::createEventTab() {
  return Mantid::Kernel::make_unique<EventView>(this);
}

Mantid::API::IAlgorithm_sptr BatchView::createReductionAlg() {
  return Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryReductionOneAuto");
}

std::unique_ptr<SaveView> BatchView::createSaveTab() {
  return Mantid::Kernel::make_unique<SaveView>(this);
}
} // namespace CustomInterfaces
} // namespace MantidQt
