// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectInterface.h"
#include "IndirectSettings.h"

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
  HelpWindow::showCustomInterface(QString::fromStdString(documentationPage()), QString("indirect"));
}

void IndirectInterface::settings() {
  auto settingsWindow = new IndirectSettings(this);
  settingsWindow->connectInterface(this);

  settingsWindow->loadSettings();
  settingsWindow->setWindowFlag(Qt::Window);
  settingsWindow->setWindowModality(Qt::WindowModal);
  settingsWindow->show();
}

void IndirectInterface::applySettings() { applySettings(IndirectSettings::getSettings()); }

void IndirectInterface::applySettings(std::map<std::string, QVariant> const &settings) { UNUSED_ARG(settings); }

void IndirectInterface::manageUserDirectories() { ManageUserDirectories::openManageUserDirectories(); }

void IndirectInterface::showMessageBox(QString const &message) { showInformationBox(message); }

} // namespace MantidQt::CustomInterfaces
