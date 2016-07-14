#include "MantidQtCustomInterfaces/Reflectometry/QtReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflRunsTabView.h"

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
QtReflMainWindowView::~QtReflMainWindowView() {}

/**
Initialise the Interface
*/
void QtReflMainWindowView::initLayout() {
  m_ui.setupUi(this);

  // Add tabs

  // 'Runs' tab

  QtReflRunsTabView *runsTab = new QtReflRunsTabView(this);
  runsTab->initLayout();
  m_ui.mainTab->addTab(runsTab, QString("Runs"));

  // This tab may need to run python code (to import/export TBL and to search
  // the ICAT). The corresponding signal needs to be re-emitted by this widget
  connect(runsTab, SIGNAL(runAsPythonScript(const QString &, bool)), this,
          SIGNAL(runAsPythonScript(const QString &, bool)));
}
}
}
