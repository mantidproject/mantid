#include "MantidQtCustomInterfaces/Reflectometry/QtReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflRunsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflSettingsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainWindowPresenter.h"

namespace MantidQt {
namespace CustomInterfaces {

DECLARE_SUBWINDOW(QtReflMainWindowView)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
QtReflMainWindowView::QtReflMainWindowView(QWidget *parent)
    : UserSubWindow(parent) {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
QtReflMainWindowView::~QtReflMainWindowView() { delete m_presenter; }

/**
Initialise the Interface
*/
void QtReflMainWindowView::initLayout() {
  m_ui.setupUi(this);

  // Create the tabs
  auto runsPresenter = createRunsTab();
  auto settingsPresenter = createSettingsTab();

  // Create the presenter
  m_presenter =
      new ReflMainWindowPresenter(this, runsPresenter, settingsPresenter);
}

/** Creates the 'Runs' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Runs' tab
*/
IReflRunsTabPresenter *QtReflMainWindowView::createRunsTab() {

  QtReflRunsTabView *runsTab = new QtReflRunsTabView(this);
  m_ui.mainTab->addTab(runsTab, QString("Runs"));

  // This tab may need to run python code (to import/export TBL and to search
  // the ICAT). The corresponding signal needs to be re-emitted by this widget
  // so the python code is executed
  connect(runsTab, SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));

  return runsTab->getPresenter();
}

/** Creates the 'Settings' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Settings' tab
*/
IReflSettingsTabPresenter *QtReflMainWindowView::createSettingsTab() {

  QtReflSettingsTabView *settingsTab = new QtReflSettingsTabView(this);
  m_ui.mainTab->addTab(settingsTab, QString("Settings"));

  return settingsTab->getPresenter();
}
}
}
