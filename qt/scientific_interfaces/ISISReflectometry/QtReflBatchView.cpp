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

//----------------------------------------------------------------------------------------------
/** Constructor
*/
QtReflBatchView::QtReflBatchView(QWidget *parent) : QWidget(parent) {
  initLayout();
}

/**
Initialise the Interface
*/
void QtReflBatchView::initLayout() {
  m_ui.setupUi(this);

  // Create the tabs
  auto runsPresenter = createRunsTab();
  auto eventPresenter = createEventTab();
  auto settingsPresenter = createSettingsTab();
  auto savePresenter = createSaveTab();

  // Create the presenter
  m_presenter = Mantid::Kernel::make_unique<ReflBatchPresenter>(
      this, std::move(runsPresenter), eventPresenter, settingsPresenter,
      std::move(savePresenter));
}

int indexOfElseFirst(std::string const &instrument,
                     std::vector<std::string> const &instruments) {
  auto it = std::find(instruments.cbegin(), instruments.cend(), instrument);
  if (it != instruments.cend())
    return static_cast<int>(std::distance(instruments.cbegin(), it));
  else
    return 0;
}

int defaultInstrumentFromConfig(std::vector<std::string> const &instruments) {
  return indexOfElseFirst(
      Mantid::Kernel::ConfigService::Instance().getString("default.instrument"),
      instruments);
}

/** Creates the 'Runs' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Runs' tab
*/
std::unique_ptr<IReflRunsTabPresenter> QtReflBatchView::createRunsTab() {
  auto instruments = std::vector<std::string>(
      {{"INTER", "SURF", "CRISP", "POLREF", "OFFSPEC"}});
  auto defaultInstrumentIndex = defaultInstrumentFromConfig(instruments);

  auto *runsTab = new QtReflRunsTabView(this, BatchViewFactory(instruments));
  m_ui.batchTabs->addTab(runsTab, QString("Runs"));
  connect(runsTab, SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  auto workspaceNamesFactory = WorkspaceNamesFactory(Slicing());
  auto runsTabPresenter = Mantid::Kernel::make_unique<ReflRunsTabPresenter>(
      runsTab, runsTab,
      BatchPresenterFactory(instruments, 0.01, workspaceNamesFactory),
      workspaceNamesFactory, 0.01, instruments, defaultInstrumentIndex);

  return std::move(runsTabPresenter);
}

/** Creates the 'Event Handling' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Event Handling' tab
*/
IReflEventTabPresenter *QtReflBatchView::createEventTab() {

  QtReflEventTabView *eventTab = new QtReflEventTabView(this);
  m_ui.batchTabs->addTab(eventTab, QString("Event Handling"));

  return eventTab->getPresenter();
}

/** Creates the 'Settings' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Settings' tab
*/
IReflSettingsTabPresenter *QtReflBatchView::createSettingsTab() {

  QtReflSettingsTabView *settingsTab = new QtReflSettingsTabView(this);
  m_ui.batchTabs->addTab(settingsTab, QString("Settings"));

  return settingsTab->getPresenter();
}

/** Creates the 'Save ASCII' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Save ASCII' tab
*/
std::unique_ptr<IReflSaveTabPresenter> QtReflBatchView::createSaveTab() {
  auto saveTabView = Mantid::Kernel::make_unique<QtReflSaveTabView>(this);
  m_ui.batchTabs->addTab(saveTabView.get(), QString("Save ASCII"));

  auto saver = Mantid::Kernel::make_unique<ReflAsciiSaver>();
  return Mantid::Kernel::make_unique<ReflSaveTabPresenter>(
      std::move(saver), std::move(saveTabView));
}

}
}
