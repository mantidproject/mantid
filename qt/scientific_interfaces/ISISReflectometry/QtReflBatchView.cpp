#include "QtReflBatchView.h"
#include "QtReflEventTabView.h"
#include "QtReflRunsTabView.h"
#include "QtReflSaveTabView.h"
#include "QtReflSettingsTabView.h"
#include "ReflSaveTabPresenter.h"
#include "ReflBatchPresenter.h"
#include "ReflAsciiSaver.h"
#include "MantidKernel/make_unique.h"
#include "Presenters/BatchPresenter.h"
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
  m_ui.batchTabs->addTab(m_settings.get(), "Save ASCII");
}

IReflRunsTabView *QtReflBatchView::runs() const { return m_runs.get(); }

IReflEventTabView *QtReflBatchView::eventHandling() const {
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
      this, BatchViewFactory(instruments));
}

std::unique_ptr<QtReflEventTabView> QtReflBatchView::createEventTab() {
  return Mantid::Kernel::make_unique<QtReflEventTabView>(this);
}

std::unique_ptr<QtReflSettingsTabView> QtReflBatchView::createSettingsTab() {
  return Mantid::Kernel::make_unique<QtReflSettingsTabView>(this);
}

std::unique_ptr<QtReflSaveTabView> QtReflBatchView::createSaveTab() {
  return Mantid::Kernel::make_unique<QtReflSaveTabView>(this);
}
}
}
