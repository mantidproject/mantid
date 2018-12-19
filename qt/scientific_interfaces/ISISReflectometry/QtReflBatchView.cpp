// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "QtReflBatchView.h"
#include "GUI/Event/EventView.h"
#include "GUI/Runs/RunsView.h"
#include "GUI/Save/SaveView.h"
#include "MantidAPI/AlgorithmManager.h"
#include "MantidKernel/make_unique.h"
#include "ReflAsciiSaver.h"
#include "ReflBatchPresenter.h"

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {

QtReflBatchView::QtReflBatchView(QWidget *parent) : QWidget(parent) {
  initLayout();
}

void QtReflBatchView::initLayout() {
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

IExperimentView *QtReflBatchView::experiment() const {
  return m_experiment.get();
}

IInstrumentView *QtReflBatchView::instrument() const {
  return m_instrument.get();
}

IRunsView *QtReflBatchView::runs() const { return m_runs.get(); }

IEventView *QtReflBatchView::eventHandling() const {
  return m_eventHandling.get();
}

ISaveView *QtReflBatchView::save() const { return m_save.get(); }

std::unique_ptr<RunsView> QtReflBatchView::createRunsTab() {
  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  return Mantid::Kernel::make_unique<RunsView>(
      this, RunsTableViewFactory(instruments));
}

std::unique_ptr<EventView> QtReflBatchView::createEventTab() {
  return Mantid::Kernel::make_unique<EventView>(this);
}

Mantid::API::IAlgorithm_sptr QtReflBatchView::createReductionAlg() {
  return Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryReductionOneAuto");
}

std::unique_ptr<SaveView> QtReflBatchView::createSaveTab() {
  return Mantid::Kernel::make_unique<SaveView>(this);
}
} // namespace CustomInterfaces
} // namespace MantidQt
