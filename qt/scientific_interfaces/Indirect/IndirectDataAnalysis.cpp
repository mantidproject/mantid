// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectDataAnalysis.h"

#include "ConvFit.h"
#include "Elwin.h"
#include "Iqt.h"
#include "IqtFit.h"
#include "JumpFit.h"
#include "MSDFit.h"

namespace MantidQt {
namespace CustomInterfaces {
namespace IDA {
DECLARE_SUBWINDOW(IndirectDataAnalysis)

IndirectDataAnalysis::IndirectDataAnalysis(QWidget *parent)
    : IndirectInterface(parent),
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

void IndirectDataAnalysis::applySettings(
    std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
    tab->second->setPlotErrorBars(settings.at("ErrorBars").toBool());
  }
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
  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this,
          SLOT(manageUserDirectories()));

  // Needed to initially apply the settings loaded on the settings GUI
  applySettings(getInterfaceSettings());
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
void IndirectDataAnalysis::tabChanged(int index) {}

std::string IndirectDataAnalysis::documentationPage() const {
  return "Indirect Data Analysis";
}

/**
 * Handles exporting a Python script for the current tab.
 */
void IndirectDataAnalysis::exportTabPython() {
  unsigned int currentTab = m_uiForm.twIDATabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

} // namespace IDA
} // namespace CustomInterfaces
} // namespace MantidQt
