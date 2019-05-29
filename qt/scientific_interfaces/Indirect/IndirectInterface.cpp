// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectInterface.h"

#include "MantidQtWidgets/Common/HelpWindow.h"
#include "MantidQtWidgets/Common/InterfaceManager.h"
#include "MantidQtWidgets/Common/ManageUserDirectories.h"

using namespace MantidQt::API;

namespace MantidQt {
namespace CustomInterfaces {

IndirectInterface::IndirectInterface(QWidget *parent)
    : UserSubWindow(parent),
      m_settings(dynamic_cast<IndirectSettings *>(
          InterfaceManager().createSubWindow("Settings"))) {

  connect(m_settings.get(), SIGNAL(applySettings()), this,
          SLOT(applySettings()));
}

void IndirectInterface::help() {
  HelpWindow::showCustomInterface(nullptr,
                                  QString::fromStdString(documentationPage()));
}

void IndirectInterface::settings() {
  m_settings->loadSettings();
  m_settings->show();
  m_settings->setFocus();
}

void IndirectInterface::applySettings() {
  applySettings(getInterfaceSettings());
}

void IndirectInterface::applySettings(
    std::map<std::string, QVariant> const &settings) {
  UNUSED_ARG(settings);
}

std::map<std::string, QVariant>
IndirectInterface::getInterfaceSettings() const {
  return m_settings->getSettings();
}

void IndirectInterface::manageUserDirectories() {
  ManageUserDirectories *ad = new ManageUserDirectories(this);
  ad->show();
  ad->setFocus();
}

void IndirectInterface::showMessageBox(QString const &message) {
  showInformationBox(message);
}

} // namespace CustomInterfaces
} // namespace MantidQt
