// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------
// Includes
//----------------------
#include "IndirectDataAnalysis.h"

#include "ConvFit.h"
#include "Elwin.h"
#include "Iqt.h"
#include "IqtFit.h"
#include "JumpFit.h"
#include "MSDFit.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"

#include "MantidAPI/AnalysisDataService.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
// Add this class to the list of specialised dialogs in this namespace
DECLARE_SUBWINDOW(IndirectDataAnalysis)

/**
 * Constructor.
 *
 * @param parent :: the parent QWidget.
 */
IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent)
    : UserSubWindow(parent),
      m_settingsPresenter(
          Mantid::Kernel::make_unique<IndirectSettingsPresenter>(
              this, "Data Analysis", "restrict-input-by-name,plot-error-bars")),
      m_settingsGroup("CustomInterfaces/IndirectAnalysis/"), m_valInt(nullptr),
      m_valDbl(nullptr),
      m_changeObserver(*this, &IndirectDataAnalysis::handleDirectoryChange) {
  m_uiForm.setupUi(this);

  // Allows us to get a handle on a tab using an enum, for example
  // "m_tabs[ELWIN]".
  // All tabs MUST appear here to be shown in interface.
  // We make the assumption that each map key corresponds to the order in which
  // the tabs appear.
  m_tabs.emplace(ELWIN, new Elwin(m_uiForm.twIDATabs->widget(ELWIN)));
  m_tabs.emplace(MSD_FIT, new MSDFit(m_uiForm.twIDATabs->widget(MSD_FIT)));
  m_tabs.emplace(IQT, new Iqt(m_uiForm.twIDATabs->widget(IQT)));
  m_tabs.emplace(IQT_FIT, new IqtFit(m_uiForm.twIDATabs->widget(IQT_FIT)));
  m_tabs.emplace(CONV_FIT, new ConvFit(m_uiForm.twIDATabs->widget(CONV_FIT)));
  m_tabs.emplace(JUMP_FIT, new JumpFit(m_uiForm.twIDATabs->widget(JUMP_FIT)));
}

/**
 * @param :: the detected close event
 */
void IndirectDataAnalysis::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void IndirectDataAnalysis::handleDirectoryChange(
    Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();

  if (key == "defaultsave.directory")
    loadSettings();
}

/**
 * Initialised the layout of the interface.  MUST be called.
 */
void IndirectDataAnalysis::initLayout() {
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

  connect(m_uiForm.twIDATabs, SIGNAL(currentChanged(int)), this,
          SLOT(tabChanged(int)));
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this,
          SLOT(exportTabPython()));
  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this,
          SLOT(settingsClicked()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(openDirectoryDialog()));

  connect(m_settingsPresenter.get(), SIGNAL(applySettings()), this,
          SLOT(applySettings()));

  // Needed to initially apply the settings loaded on the settings GUI
  applySettings();
}

/**
 * Allow Python to be called locally.
 */
void IndirectDataAnalysis::initLocalPython() {
  QString pyInput = "from mantid.simpleapi import *";
  QString pyOutput = runPythonCode(pyInput).trimmed();
  loadSettings();
}

/**
 * Load the settings saved for this interface.
 */
void IndirectDataAnalysis::loadSettings() {
  auto const saveDir = Mantid::Kernel::ConfigService::Instance().getString(
      "defaultsave.directory");

  QSettings settings;
  settings.beginGroup(m_settingsGroup + "ProcessedFiles");

  settings.setValue("last_directory", QString::fromStdString(saveDir));

  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab)
    tab->second->loadTabSettings(settings);

  settings.endGroup();
}

/**
 * Sets the active workspace in the selected tab
 */
void IndirectDataAnalysis::tabChanged(int index) {
  m_tabs[index]->setActiveWorkspace();
}

/**
 * Opens a directory dialog.
 */
void IndirectDataAnalysis::openDirectoryDialog() {
  auto ad = new MantidQt::API::ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

/**
 * Opens the settings dialog.
 */
void IndirectDataAnalysis::settingsClicked() {
  m_settingsPresenter->loadSettings();
  m_settingsPresenter->showDialog();
}

/**
 * Opens the Mantid Wiki web page of the current tab.
 */
void IndirectDataAnalysis::help() {
  MantidQt::API::HelpWindow::showCustomInterface(
      nullptr, QString("Indirect Data Analysis"));
}

/**
 * Handles exporting a Python script for the current tab.
 */
void IndirectDataAnalysis::exportTabPython() {
  unsigned int currentTab = m_uiForm.twIDATabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

/**
 * Updates the settings decided on the Settings Dialog
 */
void IndirectDataAnalysis::applySettings() {
  auto const restrict =
      m_settingsPresenter->getSetting("restrict-input-by-name").toBool();
  auto const errorBars =
      m_settingsPresenter->getSetting("plot-error-bars").toBool();

  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->filterInputData(restrict);
    tab->second->setPlotErrorBars(errorBars);
  }
}

/**
 * Slot to wrap the protected showInformationBox method defined
 * in UserSubWindow and provide access to composed tabs.
 *
 * @param message The message to display in the message box
 */
void IndirectDataAnalysis::showMessageBox(const QString &message) {
  showInformationBox(message);
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
