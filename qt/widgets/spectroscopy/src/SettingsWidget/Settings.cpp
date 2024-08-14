// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/Settings.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"
#include "MantidQtWidgets/Spectroscopy/InelasticInterface.h"
#include "MantidQtWidgets/Spectroscopy/SettingsWidget/SettingsHelper.h"

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

void Settings::connectExistingInterfaces(QList<QPointer<MantidQt::API::UserSubWindow>> const &windows) {
  for (auto const &window : windows) {
    if (auto inelasticInterface = dynamic_cast<InelasticInterface *>(window.data())) {
      connect(this, SIGNAL(applySettings()), inelasticInterface, SLOT(applySettings()));
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
