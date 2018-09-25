#include "QtReflBatchView.h"
#include "GUI/Event/EventView.h"
#include "QtReflRunsTabView.h"
#include "QtReflSaveTabView.h"
#include "ReflSaveTabPresenter.h"
#include "ReflBatchPresenter.h"
#include "ReflAsciiSaver.h"
#include "MantidKernel/make_unique.h"
#include "MantidAPI/AlgorithmManager.h"
#include "ReflRunsTabPresenter.h"

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

  m_save = createSaveTab();
  m_ui.batchTabs->addTab(m_save.get(), "Save ASCII");

  m_experiment = Mantid::Kernel::make_unique<ExperimentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_experiment.get(), "Experiment Settings");

  m_instrument =
      Mantid::Kernel::make_unique<InstrumentView>(createReductionAlg(), this);
  m_ui.batchTabs->addTab(m_instrument.get(), "Instrument Settings");
}

IExperimentView* QtReflBatchView::experiment() const {
  return m_experiment.get();
}

IInstrumentView *QtReflBatchView::instrument() const {
  return m_instrument.get();
}

IReflRunsTabView *QtReflBatchView::runs() const { return m_runs.get(); }

IEventView *QtReflBatchView::eventHandling() const {
  return m_eventHandling.get();
}

IReflSaveTabView *QtReflBatchView::save() const { return m_save.get(); }

std::unique_ptr<QtReflRunsTabView> QtReflBatchView::createRunsTab() {
  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  return Mantid::Kernel::make_unique<QtReflRunsTabView>(
      this, RunsTableViewFactory(instruments));
}

std::unique_ptr<EventView> QtReflBatchView::createEventTab() {
  return Mantid::Kernel::make_unique<EventView>(this);
}

Mantid::API::IAlgorithm_sptr QtReflBatchView::createReductionAlg() {
  return Mantid::API::AlgorithmManager::Instance().create(
      "ReflectometryReductionOneAuto");
}

std::unique_ptr<QtReflSaveTabView> QtReflBatchView::createSaveTab() {
  return Mantid::Kernel::make_unique<QtReflSaveTabView>(this);
}
}
}
