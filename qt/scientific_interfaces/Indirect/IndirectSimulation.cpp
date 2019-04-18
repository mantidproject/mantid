// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSimulation.h"
#include "DensityOfStates.h"
#include "IndirectMolDyn.h"
#include "IndirectSassena.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"

// Add this class to the list of specialised dialogs in this namespace
namespace MantidQt {
namespace CustomInterfaces {
DECLARE_SUBWINDOW(IndirectSimulation)
}
} // namespace MantidQt

using namespace MantidQt::CustomInterfaces;

IndirectSimulation::IndirectSimulation(QWidget *parent)
    : UserSubWindow(parent),
      m_settingsPresenter(
          Mantid::Kernel::make_unique<IDA::IndirectSettingsPresenter>(this)),
      m_changeObserver(*this, &IndirectSimulation::handleDirectoryChange) {}

IndirectSimulation::~IndirectSimulation() {}

void IndirectSimulation::initLayout() {
  m_uiForm.setupUi(this);

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Insert each tab into the interface on creation
  m_simulationTabs.emplace(
      MOLDYN,
      new IndirectMolDyn(m_uiForm.IndirectSimulationTabs->widget(MOLDYN)));
  m_simulationTabs.emplace(
      SASSENA,
      new IndirectSassena(m_uiForm.IndirectSimulationTabs->widget(SASSENA)));
  m_simulationTabs.emplace(
      DOS, new DensityOfStates(m_uiForm.IndirectSimulationTabs->widget(DOS)));

  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, IndirectSimulationTab *>::iterator iter;
  for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end();
       ++iter) {
    connect(iter->second, SIGNAL(runAsPythonScript(const QString &, bool)),
            this, SIGNAL(runAsPythonScript(const QString &, bool)));
    connect(iter->second, SIGNAL(showMessageBox(const QString &)), this,
            SLOT(showMessageBox(const QString &)));
  }

  loadSettings();

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this,
          SLOT(settingsClicked()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(helpClicked()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(manageUserDirectories()));
}

/**
 * Handles closing the window.
 *
 * @param :: the detected close event
 */
void IndirectSimulation::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void IndirectSimulation::handleDirectoryChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();
  if (key == "defaultsave.directory") {
    loadSettings();
  }
}

/**
 * Load the setting for each tab on the interface.
 *
 * This includes setting the default browsing directory to be the default save
 *directory.
 */
void IndirectSimulation::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir = QString::fromStdString(
      Mantid::Kernel::ConfigService::Instance().getString(
          "defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  std::map<unsigned int, IndirectSimulationTab *>::iterator iter;
  for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end();
       ++iter) {
    iter->second->loadSettings(settings);
  }

  settings.endGroup();
}

/**
 * Opens the settings dialog
 */
void IndirectSimulation::settingsClicked() {
  m_settingsPresenter->loadSettings();
  m_settingsPresenter->showDialog();
}

/**
 * Slot to open a new browser window and navigate to the help page
 * on the wiki for the currently selected tab.
 */
void IndirectSimulation::helpClicked() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Indirect Simulation"));
}

/**
 * Slot to show the manage user dicrectories dialog when the user clicks
 * the button on the interface.
 */
void IndirectSimulation::manageUserDirectories() {
  MantidQt::API::ManageUserDirectories *ad =
      new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 *
 * @param message :: The message to display in the message box
 */
void IndirectSimulation::showMessageBox(const QString &message) {
  showInformationBox(message);
}
