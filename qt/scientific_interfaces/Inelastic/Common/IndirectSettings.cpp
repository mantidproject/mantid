// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "IndirectSettings.h"
#include "IndirectInterface.h"
#include "IndirectSettingsHelper.h"
#include "MantidQtIcons/Icon.h"
#include "MantidQtWidgets/Common/UserSubWindow.h"

constexpr auto SETTINGS_ICON = "mdi.settings";

namespace MantidQt::CustomInterfaces {

IndirectSettings::IndirectSettings(QWidget *parent) : QWidget(parent) {
  this->setWindowTitle("Interface Settings");

  auto model = std::make_unique<IndirectSettingsModel>();
  m_presenter = std::make_unique<IndirectSettingsPresenter>(std::move(model), new IndirectSettingsView(this));
  m_presenter->subscribeParent(this);

  auto layout = new QGridLayout();
  layout->addWidget(m_presenter->getView());
  setLayout(layout);
}

void IndirectSettings::connectExistingInterfaces(QList<QPointer<MantidQt::API::UserSubWindow>> &windows) {
  for (auto const &window : windows) {
    if (auto indirectInterface = dynamic_cast<IndirectInterface *>(window.data())) {
      connect(this, SIGNAL(applySettings()), indirectInterface, SLOT(applySettings()));
    }
  }
}

QIcon IndirectSettings::icon() { return Icons::getIcon(SETTINGS_ICON); }

std::map<std::string, QVariant> IndirectSettings::getSettings() {
  std::map<std::string, QVariant> interfaceSettings;
  interfaceSettings["RestrictInput"] = IndirectSettingsHelper::restrictInputDataByName();
  interfaceSettings["ErrorBars"] = IndirectSettingsHelper::externalPlotErrorBars();
  return interfaceSettings;
}

void IndirectSettings::notifyApplySettings() { emit applySettings(); }

void IndirectSettings::notifyCloseSettings() {
  if (auto settingsWindow = window())
    settingsWindow->close();
}

void IndirectSettings::loadSettings() { m_presenter->loadSettings(); }

} // namespace MantidQt::CustomInterfaces
