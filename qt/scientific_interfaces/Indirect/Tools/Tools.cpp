// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Tools.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"
#include "TransmissionCalc.h"

#include "MantidKernel/ConfigService.h"

namespace MantidQt::CustomInterfaces {
DECLARE_SUBWINDOW(Tools)
} // namespace MantidQt::CustomInterfaces

using namespace MantidQt::CustomInterfaces;

Tools::Tools(QWidget *parent) : InelasticInterface(parent), m_changeObserver(*this, &Tools::handleDirectoryChange) {}

void Tools::initLayout() {
  m_uiForm.setupUi(this);
  m_uiForm.pbSettings->setIcon(Settings::icon());

  // Connect Poco Notification Observer
  Mantid::Kernel::ConfigService::Instance().addObserver(m_changeObserver);

  // Insert each tab into the interface on creation
  m_tabs.emplace(TRANSMISSION, new TransmissionCalc(m_uiForm.ToolsTabs->widget(TRANSMISSION)));

  // Connect each tab to the actions available in this GUI
  std::map<unsigned int, ToolsTab *>::iterator iter;
  for (iter = m_tabs.begin(); iter != m_tabs.end(); ++iter) {
    connect(iter->second, &ToolsTab::showMessageBox, this, &Tools::showMessageBox);
  }

  loadSettings();

  connect(m_uiForm.pbSettings, &QPushButton::clicked, this, &Tools::settings);
  connect(m_uiForm.pbHelp, &QPushButton::clicked, this, &Tools::help);
  connect(m_uiForm.pbManageDirs, &QPushButton::clicked, this, &Tools::manageUserDirectories);
}

/**
 * Handles closing the window.
 *
 * @param :: the detected close event
 */
void Tools::closeEvent(QCloseEvent * /*unused*/) {
  Mantid::Kernel::ConfigService::Instance().removeObserver(m_changeObserver);
}

/**
 * Handles a change in directory.
 *
 * @param pNf :: notification
 */
void Tools::handleDirectoryChange(Mantid::Kernel::ConfigValChangeNotification_ptr pNf) {
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
void Tools::loadSettings() {
  QSettings settings;
  QString settingsGroup = "CustomInterfaces/IndirectAnalysis/";
  QString saveDir =
      QString::fromStdString(Mantid::Kernel::ConfigService::Instance().getString("defaultsave.directory"));

  settings.beginGroup(settingsGroup + "ProcessedFiles");
  settings.setValue("last_directory", saveDir);

  std::map<unsigned int, ToolsTab *>::iterator iter;
  for (iter = m_tabs.begin(); iter != m_tabs.end(); ++iter) {
    iter->second->loadSettings(settings);
  }

  settings.endGroup();
}

std::string Tools::documentationPage() const { return "Indirect Tools"; }

Tools::~Tools() = default;
