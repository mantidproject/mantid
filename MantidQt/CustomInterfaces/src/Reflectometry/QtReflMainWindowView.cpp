#include "MantidQtCustomInterfaces/Reflectometry/QtReflMainWindowView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflEventTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflRunsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflSaveTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/QtReflSettingsTabView.h"
#include "MantidQtCustomInterfaces/Reflectometry/ReflMainWindowPresenter.h"

#include <qinputdialog.h>
#include <qmessagebox.h>

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

  // Create the tabs
  auto runsPresenter = createRunsTab();
  auto eventPresenter = createEventTab();
  auto settingsPresenter = createSettingsTab();
  auto savePresenter = createSaveTab();

  // Create the presenter
  m_presenter.reset(new ReflMainWindowPresenter(
      this, runsPresenter, eventPresenter, settingsPresenter, savePresenter));
}

/** Creates the 'Runs' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Runs' tab
*/
IReflRunsTabPresenter *QtReflMainWindowView::createRunsTab() {

  QtReflRunsTabView *runsTab = new QtReflRunsTabView(this);
  m_ui.mainTab->addTab(runsTab, QString("Runs"));

  return runsTab->getPresenter();
}

/** Creates the 'Event Handling' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Event Handling' tab
*/
IReflEventTabPresenter *QtReflMainWindowView::createEventTab() {

  QtReflEventTabView *eventTab = new QtReflEventTabView(this);
  m_ui.mainTab->addTab(eventTab, QString("Event Handling"));

  return eventTab->getPresenter();
}

/** Creates the 'Settings' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Settings' tab
*/
IReflSettingsTabPresenter *QtReflMainWindowView::createSettingsTab() {

  QtReflSettingsTabView *settingsTab = new QtReflSettingsTabView(this);
  m_ui.mainTab->addTab(settingsTab, QString("Settings"));

  return settingsTab->getPresenter();
}

/** Creates the 'Save ASCII' tab and returns a pointer to its presenter
* @return :: A pointer to the presenter managing the 'Save ASCII' tab
*/
IReflSaveTabPresenter *QtReflMainWindowView::createSaveTab() {

  QtReflSaveTabView *saveTab = new QtReflSaveTabView(this);
  m_ui.mainTab->addTab(saveTab, QString("Save ASCII"));

  return saveTab->getPresenter();
}

/**
Show an critical error dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflMainWindowView::giveUserCritical(const std::string &prompt,
                                            const std::string &title) {
  QMessageBox::critical(this, QString::fromStdString(title),
                        QString::fromStdString(prompt), QMessageBox::Ok,
                        QMessageBox::Ok);
}

/**
Show a warning dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflMainWindowView::giveUserWarning(const std::string &prompt,
                                           const std::string &title) {
  QMessageBox::warning(this, QString::fromStdString(title),
                       QString::fromStdString(prompt), QMessageBox::Ok,
                       QMessageBox::Ok);
}

/**
Show an information dialog
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
*/
void QtReflMainWindowView::giveUserInfo(const std::string &prompt,
                                        const std::string &title) {
  QMessageBox::information(this, QString::fromStdString(title),
                           QString::fromStdString(prompt), QMessageBox::Ok,
                           QMessageBox::Ok);
}

/**
Ask the user a Yes/No question
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@returns a boolean true if Yes, false if No
*/
bool QtReflMainWindowView::askUserYesNo(const std::string &prompt,
                                        const std::string &title) {
  auto response = QMessageBox::question(
      this, QString::fromStdString(title), QString::fromStdString(prompt),
      QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
  if (response == QMessageBox::Yes) {
    return true;
  }
  return false;
}

/**
Ask the user to enter a string.
@param prompt : The prompt to appear on the dialog
@param title : The text for the title bar of the dialog
@param defaultValue : The default value entered.
@returns The user's string if submitted, or an empty string
*/
std::string
QtReflMainWindowView::askUserString(const std::string &prompt,
                                    const std::string &title,
                                    const std::string &defaultValue) {
  bool ok;
  QString text = QInputDialog::getText(
      this, QString::fromStdString(title), QString::fromStdString(prompt),
      QLineEdit::Normal, QString::fromStdString(defaultValue), &ok);
  if (ok)
    return text.toStdString();
  return "";
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
}
}
