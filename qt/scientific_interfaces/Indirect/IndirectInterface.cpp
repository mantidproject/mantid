// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectInterface.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"

using namespace MantidQt::API;

namespace MantidQt::CustomInterfaces {

IndirectInterface::IndirectInterface(QWidget *parent) : UserSubWindow(parent), m_settings() {}

void IndirectInterface::help() {
  HelpWindow::showCustomInterface(QString::fromStdString(documentationPage()), QString("indirect"));
}

void IndirectInterface::settings() {
  auto subWindow = InterfaceManager().createSubWindow("Settings", this);
  m_settings = dynamic_cast<IndirectSettings *>(subWindow);
  connect(m_settings, SIGNAL(applySettings()), this, SLOT(applySettings()));
  connect(m_settings, SIGNAL(closeSettings()), this, SLOT(closeSettings()));

  m_settings->loadSettings();
  m_settings->setWindowModality(Qt::WindowModal);
  m_settings->show();
}

void IndirectInterface::applySettings() { applySettings(getInterfaceSettings()); }

void IndirectInterface::applySettings(std::map<std::string, QVariant> const &settings) { UNUSED_ARG(settings); }

void IndirectInterface::closeSettings() {
  disconnect(m_settings, SIGNAL(applySettings()), this, SLOT(applySettings()));
  disconnect(m_settings, SIGNAL(closeSettings()), this, SLOT(closeSettings()));

  if (auto settingsWindow = m_settings->window())
    settingsWindow->close();

  m_settings = nullptr;
}

std::map<std::string, QVariant> IndirectInterface::getInterfaceSettings() const { return m_settings->getSettings(); }

void IndirectInterface::manageUserDirectories() { ManageUserDirectories::openManageUserDirectories(); }

void IndirectInterface::showMessageBox(QString const &message) { showInformationBox(message); }

} // namespace MantidQt::CustomInterfaces
