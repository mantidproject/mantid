#include "QtReflMainWindowView.h"
#include "QtReflEventTabView.h"
#include "QtReflRunsTabView.h"
#include "QtReflSaveTabView.h"
#include "QtReflSettingsTabView.h"
#include "ReflSaveTabPresenter.h"
#include "ReflMainWindowPresenter.h"
#include "ReflAsciiSaver.h"
#include "MantidKernel/make_unique.h"
#include "Presenters/BatchPresenter.h"
#include "ReflRunsTabPresenter.h"

#include <QMessageBox>

namespace MantidQt {
namespace CustomInterfaces {

DECLARE_SUBWINDOW(QtReflMainWindowView)

QtReflMainWindowView::QtReflMainWindowView(QWidget *parent)
    : UserSubWindow(parent) {}

/**
Initialise the Interface
*/
void QtReflMainWindowView::initLayout() {
  m_ui.setupUi(this);
  connect(m_ui.helpButton, SIGNAL(clicked()), this, SLOT(helpPressed()));

  // Create the presenter
  m_presenter = Mantid::Kernel::make_unique<ReflMainWindowPresenter>(this);
}

void QtReflMainWindowView::helpPressed() {
  m_presenter->notify(IReflMainWindowPresenter::Flag::HelpPressed);
}

/**
Runs python code
* @param pythonCode : [input] The code to run
* @return : Result of the execution
*/
std::string
QtReflMainWindowView::runPythonAlgorithm(const std::string &pythonCode) {

  QString output = runPythonCode(QString::fromStdString(pythonCode), false);
  return output.toStdString();
}

/**
Handles attempt to close main window
* @param event : [input] The close event
*/
void QtReflMainWindowView::closeEvent(QCloseEvent *event) {
  // Close only if reduction has been paused
  if (!m_presenter->isProcessing()) {
    event->accept();
  } else {
    event->ignore();
  }
}
}
}
