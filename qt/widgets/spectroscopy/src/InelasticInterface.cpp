// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/InelasticInterface.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

using namespace MantidQt::API;

namespace MantidQt::CustomInterfaces {

InelasticInterface::InelasticInterface(QWidget *parent) : UserSubWindow(parent) {}

void InelasticInterface::initLayout() {
  // Needed to initially apply the settings loaded on the settings GUI
  applySettings();
}

void InelasticInterface::help() {
  auto const docPageName = QString::fromStdString(documentationPage());
  // Extract the category
  auto const category = docPageName.left(docPageName.indexOf(' ')).toLower();

  HelpWindow::showCustomInterface(docPageName, category);
}

void InelasticInterface::settings() {
  auto settingsWidget = new Settings(this);
  settingsWidget->connectExistingInterfaces(InterfaceManager::existingInterfaces());

  settingsWidget->loadSettings();
  settingsWidget->setAttribute(Qt::WA_DeleteOnClose);
  settingsWidget->setWindowFlag(Qt::Window);
  settingsWidget->setWindowModality(Qt::WindowModal);
  settingsWidget->show();
}

void InelasticInterface::applySettings() { applySettings(Settings::getSettings()); }

void InelasticInterface::applySettings(std::map<std::string, QVariant> const &settings) { UNUSED_ARG(settings); }

void InelasticInterface::manageUserDirectories() { ManageUserDirectories::openManageUserDirectories(); }

void InelasticInterface::showMessageBox(std::string const &message) {
  showInformationBox(QString::fromStdString(message));
}

} // namespace MantidQt::CustomInterfaces
