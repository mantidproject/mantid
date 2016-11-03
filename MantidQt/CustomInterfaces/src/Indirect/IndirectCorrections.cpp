//----------------------
// Includes
//----------------------

#include "MantidQtCustomInterfaces/Indirect/AbsorptionCorrections.h"
#include "MantidQtCustomInterfaces/Indirect/ApplyPaalmanPings.h"
#include "MantidQtCustomInterfaces/Indirect/CalculatePaalmanPings.h"
#include "MantidQtCustomInterfaces/Indirect/ContainerSubtraction.h"
#include "MantidQtCustomInterfaces/Indirect/IndirectCorrections.h"

#include "MantidQtAPI/HelpWindow.h"
#include "MantidQtAPI/ManageUserDirectories.h"

#include "MantidAPI/AnalysisDataService.h"

namespace MantidQt {
namespace CustomInterfaces {
// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(IndirectCorrections)

/**
 * Constructor.
 *
 * @param parent :: the parent QWidget.
 */
IndirectCorrections::IndirectCorrections(QWidget *parent)
    : UserSubWindow(parent),
      m_changeObserver(*this, &IndirectCorrections::handleDirectoryChange) {
  m_uiForm.setupUi(this);

  // Allows us to get a handle on a tab using an enum, for example
  // "m_tabs[ELWIN]".
  // All tabs MUST appear here to be shown in interface.
  // We make the assumption that each map key corresponds to the order in which
  // the tabs appear.
  m_tabs.emplace(
      CONTAINER_SUBTRACTION,
      new ContainerSubtraction(m_uiForm.twTabs->widget(CONTAINER_SUBTRACTION)));
  m_tabs.emplace(CALC_CORR,
                 new CalculatePaalmanPings(m_uiForm.twTabs->widget(CALC_CORR)));
  m_tabs.emplace(APPLY_CORR,
                 new ApplyPaalmanPings(m_uiForm.twTabs->widget(APPLY_CORR)));
  m_tabs.emplace(ABSORPTION_CORRECTIONS,
                 new AbsorptionCorrections(
                     m_uiForm.twTabs->widget(ABSORPTION_CORRECTIONS)));
}

/**
 * @param :: the detected close event
 */
void IndirectCorrections::closeEvent(QCloseEvent *) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void IndirectCorrections::handleDirectoryChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();

  if (key == "defaultsave.directory")
    loadSettings();
}

/**
 * Initialised the layout of the interface.  MUST be called.
 */
void IndirectCorrections::initLayout() {
  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Set up all tabs
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->setupTab();
    connect(tab->second, SIGNAL(runAsPythonScript(const QString &, bool)), this,
            SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(tab->second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));
  }

  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this,
          SLOT(exportTabPython()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbRun, SIGNAL(clicked()), this, SLOT(run()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(openDirectoryDialog()));
}

/**
 * Allow Python to be called locally.
 */
void IndirectCorrections::initLocalPython() {
  QString pyInput = "from mantid.simpleapi import *";
  QString pyOutput = runPythonCode(pyInput).trimmed();
  loadSettings();
}

/**
 * Load the settings saved for this interface.
 */
void IndirectCorrections::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  // Load each tab's settings.
  auto tab = m_tabs.begin();
  for (; tab != m_tabs.end(); ++tab)
    tab->second->loadTabSettings(settings);

  settings.endGroup();
}

/**
 * Private slot, called when the Run button is pressed.  Runs current tab.
 */
void IndirectCorrections::run() {
  const unsigned int currentTab = m_uiForm.twTabs->currentIndex();
  m_tabs[currentTab]->runTab();
}

/**
 * Opens a directory dialog.
 */
void IndirectCorrections::openDirectoryDialog() {
  MantidQt::API::ManageUserDirectories *ad =
      new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Opens the Mantid Wiki web page of the current tab.
 */
void IndirectCorrections::help() {
  MantidQt::API::HelpWindow::showCustomInterface(
      NULL, QString("Indirect_Corrections"));
}

/**
 * Handles exporting a Python script for the current tab.
 */
void IndirectCorrections::exportTabPython() {
  unsigned int currentTab = m_uiForm.twTabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 *
 * @param message The message to display in the message box
 */
void IndirectCorrections::showMessageBox(const QString &message) {
  showInformationBox(message);
}

} // namespace CustomInterfaces
} // namespace MantidQt
