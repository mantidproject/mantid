// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Simulation.h"
#include "DensityOfStates.h"
#include "MantidKernel/ConfigService.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"
#include "MolDyn.h"
#include "Sassena.h"

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(Simulation)
} // namespace MantidQt::CustomInterfaces

using namespace MantidQt::CustomInterfaces;

Simulation::Simulation(QWidget *parent)
    : InelasticInterface(parent), m_changeObserver(*this, &Simulation::handleDirectoryChange) {}

Simulation::~Simulation() = default;

void Simulation::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Insert each tab into the interface on creation
  m_simulationTabs.emplace(MOLDYN, new MolDyn(m_uiForm.SimulationTabs->widget(MOLDYN)));
  m_simulationTabs.emplace(SASSENA, new Sassena(m_uiForm.SimulationTabs->widget(SASSENA)));
  m_simulationTabs.emplace(DOS, new DensityOfStates(m_uiForm.SimulationTabs->widget(DOS)));

  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, SimulationTab *>::iterator iter;
  for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end(); ++iter) {
    connect(iter->second, SIGNAL(showMessageBox(const std::string &)), this, SLOT(showMessageBox(const std::string &)));
  }

  loadSettings();

  connect(m_uiForm.pbSettings, SIGNAL(clicked()), this, SLOT(settings()));
  connect(m_uiForm.pbHelp, SIGNAL(clicked()), this, SLOT(help()));
  connect(m_uiForm.pbManageDirs, SIGNAL(clicked()), this, SLOT(manageUserDirectories()));
}

/**
 * Handles closing the window.
 *
 * @param :: the detected close event
 */
void Simulation::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void Simulation::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
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
void Simulation::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  std::map<unsigned int, SimulationTab *>::iterator iter;
  for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end(); ++iter) {
    iter->second->loadSettings(settings);
  }

  settings.endGroup();
}

void Simulation::applySettings(std::map<std::string, QVariant> const &settings) {
  std::map<unsigned int, SimulationTab *>::iterator iter;
  for (iter = m_simulationTabs.begin(); iter != m_simulationTabs.end(); ++iter) {
    iter->second->enableLoadHistoryProperty(settings.at("LoadHistory").toBool());
  }
}

std::string Simulation::documentationPage() const { return "Indirect Simulation"; }
