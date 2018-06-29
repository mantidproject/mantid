#include "QtReflBatchView.h"
#include "GUI/Event/EventView.h"
#include "QtReflRunsTabView.h"
#include "QtReflSaveTabView.h"
#include "QtReflSettingsTabView.h"
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

  m_settings = createSettingsTab();
  m_ui.batchTabs->addTab(m_settings.get(), "Settings");

  m_save = createSaveTab();
  m_ui.batchTabs->addTab(m_save.get(), "Save ASCII");
}

IReflRunsTabView *QtReflBatchView::runs() const { return m_runs.get(); }

IEventView *QtReflBatchView::eventHandling() const {
  return m_eventHandling.get();
}

IReflSettingsTabView *QtReflBatchView::settings() const {
  return m_settings.get();
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

std::unique_ptr<QtReflSettingsTabView> QtReflBatchView::createSettingsTab() {
  return Mantid::Kernel::make_unique<QtReflSettingsTabView>(
      createReductionAlg(), this);
}

std::unique_ptr<QtReflSaveTabView> QtReflBatchView::createSaveTab() {
  return Mantid::Kernel::make_unique<QtReflSaveTabView>(this);
}
}
}
