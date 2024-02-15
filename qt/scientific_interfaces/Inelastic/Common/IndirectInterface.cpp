// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectInterface.h"
#include "Settings.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

using namespace MantidQt::API;

namespace MantidQt::CustomInterfaces {

IndirectInterface::IndirectInterface(QWidget *parent) : UserSubWindow(parent) {}

void IndirectInterface::initLayout() {
  // Needed to initially apply the settings loaded on the settings GUI
  applySettings();
}

void IndirectInterface::help() {
  auto const docPageName = QString::fromStdString(documentationPage());
  // Extract the category
  auto const category = docPageName.left(docPageName.indexOf(' ')).toLower();

  HelpWindow::showCustomInterface(docPageName, category);
}

void IndirectInterface::settings() {
  auto settingsWidget = new Settings(this);
  settingsWidget->connectExistingInterfaces(InterfaceManager::existingInterfaces());

  settingsWidget->loadSettings();
  settingsWidget->setAttribute(Qt::WA_DeleteOnClose);
  settingsWidget->setWindowFlag(Qt::Window);
  settingsWidget->setWindowModality(Qt::WindowModal);
  settingsWidget->show();
}

void IndirectInterface::applySettings() { applySettings(Settings::getSettings()); }

void IndirectInterface::applySettings(std::map<std::string, QVariant> const &settings) { UNUSED_ARG(settings); }

void IndirectInterface::manageUserDirectories() { ManageUserDirectories::openManageUserDirectories(); }

void IndirectInterface::showMessageBox(QString const &message) { showInformationBox(message); }

} // namespace MantidQt::CustomInterfaces
