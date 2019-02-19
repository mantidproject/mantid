// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------

#include "IndirectCorrections.h"
#include "AbsorptionCorrections.h"
#include "ApplyAbsorptionCorrections.h"
#include "CalculatePaalmanPings.h"
#include "ContainerSubtraction.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"

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
      m_settingsPresenter(
          Mantid::Kernel::make_unique<IDA::IndirectSettingsPresenter>(
              this, "Data Corrections", "filter-input-by-name")),
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
  m_tabs.emplace(ABSORPTION_CORRECTIONS,
                 new AbsorptionCorrections(
                     m_uiForm.twTabs->widget(ABSORPTION_CORRECTIONS)));
  m_tabs.emplace(APPLY_CORR, new ApplyAbsorptionCorrections(
                                 m_uiForm.twTabs->widget(APPLY_CORR)));
}

/**
 * @param :: the detected close event
 */
void IndirectCorrections::closeEvent(QCloseEvent * /*unused*/) {
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
  for (auto &tab : m_tabs) {
    tab.second->setupTab();
    connect(tab.second, SIGNAL(runAsPythonScript(const QString &, bool)), this,
            SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(tab.second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));
  }

  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this,
          SLOT(exportTabPython()));
  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this,
          SLOT(settingsClicked()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(openDirectoryDialog()));

  connect(m_settingsPresenter.get(), SIGNAL(applySettings()), this,
          SLOT(applySettings()));
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
 * Opens a directory dialog.
 */
void IndirectCorrections::openDirectoryDialog() {
  auto ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Opens the settings dialog
 */
void IndirectCorrections::settingsClicked() {
  m_settingsPresenter->loadSettings();
  m_settingsPresenter->showDialog();
}

/**
 * Opens the Mantid Wiki web page of the current tab.
 */
void IndirectCorrections::help() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Indirect Corrections"));
}

/**
 * Handles exporting a Python script for the current tab.
 */
void IndirectCorrections::exportTabPython() {
  unsigned int currentTab = m_uiForm.twTabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

/**
 * Updates the settings decided on the Settings Dialog
 */
void IndirectCorrections::applySettings() {
  QSettings settings;
  settings.beginGroup("Data Corrections");

  auto const filter = settings.value("filter-input-by-name", true).toBool();

  settings.endGroup();

  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab)
    tab->second->filterInputData(filter);
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
