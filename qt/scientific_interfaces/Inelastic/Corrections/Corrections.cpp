// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Corrections.h"
#include "AbsorptionCorrections.h"
#include "ApplyAbsorptionCorrections.h"
#include "CalculatePaalmanPings.h"
#include "ContainerSubtraction.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(Corrections)

Corrections::Corrections(QWidget *parent)
    : InelasticInterface(parent), m_changeObserver(*this, &Corrections::handleDirectoryChange) {
  m_uiForm.setupUi(this);

  // Allows us to get a handle on a tab using an enum, for example
  // "m_tabs[ELWIN]".
  // All tabs MUST appear here to be shown in interface.
  // We make the assumption that each map key corresponds to the order in which
  // the tabs appear.
  m_tabs.emplace(CONTAINER_SUBTRACTION, new ContainerSubtraction(m_uiForm.twTabs->widget(CONTAINER_SUBTRACTION)));
  m_tabs.emplace(CALC_CORR, new CalculatePaalmanPings(m_uiForm.twTabs->widget(CALC_CORR)));
  m_tabs.emplace(ABSORPTION_CORRECTIONS, new AbsorptionCorrections(m_uiForm.twTabs->widget(ABSORPTION_CORRECTIONS)));
  m_tabs.emplace(APPLY_CORR, new ApplyAbsorptionCorrections(m_uiForm.twTabs->widget(APPLY_CORR)));
}

/**
 * @param :: the detected close event
 */
void Corrections::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void Corrections::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
  std::string key = pNf->key();

  if (key == "defaultsave.directory")
    loadSettings();
}

/**
 * Initialised the layout of the interface.  MUST be called.
 */
void Corrections::initLayout() {
  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Set up all tabs
  for (auto &tab : m_tabs) {
    connect(tab.second, SIGNAL(showMessageBox(const std::string &)), this, SLOT(showMessageBox(const std::string &)));
  }

  m_uiForm.pbSettings->setIcon(Settings::icon());
  connect(m_uiForm.pbPythonExport, SIGNAL(clicked()), this, SLOT(exportTabPython()));
  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));

  InelasticInterface::initLayout();
}

/**
 * Allow Python to be called locally.
 */
void Corrections::initLocalPython() { loadSettings(); }

/**
 * Load the settings saved for this interface.
 */
void Corrections::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  // Load each tab's settings.
  auto tab = m_tabs.begin();
  for (; tab != m_tabs.end(); ++tab)
    tab->second->loadTabSettings(settings);

  settings.endGroup();
}

void Corrections::applySettings(std::map<std::string, QVariant> const &settings) {
  for (auto tab = m_tabs.begin(); tab != m_tabs.end(); ++tab) {
    tab->second->filterInputData(settings.at("RestrictInput").toBool());
  }
}

/**
 * Handles exporting a Python script for the current tab.
 */
void Corrections::exportTabPython() {
  unsigned int currentTab = m_uiForm.twTabs->currentIndex();
  m_tabs[currentTab]->exportPythonScript();
}

std::string Corrections::documentationPage() const { return "Inelastic Corrections"; }

} // namespace MantidQt::CustomInterfaces
