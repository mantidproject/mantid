// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Corrections.h"
#include "AbsorptionCorrections.h"
#include "ApplyAbsorptionCorrections.h"
#include "ContainerSubtractionPresenter.h"
#include "ContainerSubtractionView.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"

#include <MantidQtWidgets/Common/QtJobRunner.h>

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(Corrections)

Corrections::Corrections(QWidget *parent)
    : InelasticInterface(parent), m_changeObserver(*this, &Corrections::handleDirectoryChange) {}

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
  m_uiForm.setupUi(this);

  // If all 3 tabs are MVP'd this can be refined.
  auto view = new ContainerSubtractionView(m_uiForm.tabContainerSubtraction); // lifetime handled by QWidget
  auto jobRunner = std::make_unique<MantidQt::API::QtJobRunner>(true);
  auto algorithmRunner = std::make_unique<MantidQt::API::AlgorithmRunner>(std::move(jobRunner));
  m_csPresenter =
      std::make_unique<ContainerSubtractionPresenter>(m_uiForm.tabContainerSubtraction, std::move(algorithmRunner),
                                                      std::make_unique<ContainerSubtractionModel>(), view);

  /* Using this map allows us to get a handle on a tab using an enum, for example "m_tabs[ELWIN]".
   * All tabs MUST appear here to be shown in the interface.
   * We make the assumption that each map key corresponds to the order in which the tabs appear.*/
  m_tabs.emplace(CONTAINER_SUBTRACTION, m_csPresenter.get());
  m_tabs.emplace(ABSORPTION_CORRECTIONS, new AbsorptionCorrections(m_uiForm.twTabs->widget(ABSORPTION_CORRECTIONS)));
  m_tabs.emplace(APPLY_CORRECTIONS, new ApplyAbsorptionCorrections(m_uiForm.twTabs->widget(APPLY_CORRECTIONS)));

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Set up all tabs
  for (auto &[_, tab] : m_tabs) {
    connect(tab, &CorrectionsTab::showMessageBox, this, &Corrections::showMessageBox);
  }

  m_uiForm.pbSettings->setIcon(Settings::icon());
  connect(m_uiForm.pbPythonExport, &QPushButton::clicked, this, &Corrections::exportTabPython);
  connect(m_uiForm.pbSettings, &QPushButton::clicked, this, &Corrections::settings);
  connect(m_uiForm.pbHelp, &QPushButton::clicked, this, &Corrections::help);
  connect(m_uiForm.pbManageDirs, &QPushButton::clicked, this, &Corrections::manageUserDirectories);

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
    tab->second->enableLoadHistoryProperty(settings.at("LoadHistory").toBool());
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
