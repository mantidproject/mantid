// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "Settings.h"
#include "IndirectInterface.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "SettingsHelper.h"

constexpr auto SETTINGS_ICON = "mdi.settings";

namespace MantidQt::CustomInterfaces {

Settings::Settings(QWidget *parent) : QWidget(parent) {
  this->setWindowTitle("Interface Settings");

  auto model = std::make_unique<SettingsModel>();
  m_presenter = std::make_unique<SettingsPresenter>(std::move(model), new SettingsView(this));
  m_presenter->subscribeParent(this);

  auto layout = new QGridLayout();
  layout->addWidget(m_presenter->getView());
  setLayout(layout);
}

void Settings::connectExistingInterfaces(QList<QPointer<MantidQt::API::UserSubWindow>> &windows) {
  for (auto const &window : windows) {
    if (auto indirectInterface = dynamic_cast<IndirectInterface *>(window.data())) {
      connect(this, SIGNAL(applySettings()), indirectInterface, SLOT(applySettings()));
    }
  }
}

QIcon Settings::icon() { return Icons::getIcon(SETTINGS_ICON); }

std::map<std::string, QVariant> Settings::getSettings() {
  std::map<std::string, QVariant> interfaceSettings;
  interfaceSettings["RestrictInput"] = SettingsHelper::restrictInputDataByName();
  interfaceSettings["ErrorBars"] = SettingsHelper::externalPlotErrorBars();
  return interfaceSettings;
}

void Settings::notifyApplySettings() { emit applySettings(); }

void Settings::notifyCloseSettings() {
  if (auto settingsWindow = window())
    settingsWindow->close();
}

void Settings::loadSettings() { m_presenter->loadSettings(); }

} // namespace MantidQt::CustomInterfaces
